#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cxxopts.hpp>
#include <errno.h>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#include "global.hpp"
#include "lib/random.hpp"
#include "lib/ubfexec.hpp"
#include "z3.h"

using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::unique_ptr;
using std::vector;

using u64 = std::int64_t;
using u32 = std::int32_t;

struct eBPFTestOpts {
  string uuid;
  string sno;
  string output;
  u32 max_report;
  u32 report_num;
  bool verbose;
  u32 num_procs;
  u32 proc_id; // 0 for parent, 1-N for workers
  bool no_save_prog;
  bool verify_crash;
  string load_prog;
  bool de_oracle;

  static eBPFTestOpts Parse(int argc, char **argv) {
    cxxopts::Options options("bpf_test");
    options.add_options()
      ("o,output",    "Directory to save the testing eBPF program", cxxopts::value<std::string>())
      ("s,seed",      "Seed for random sampling (negative for true random)", cxxopts::value<int>()->default_value("-1"))
      ("max_report",  "Maximum number of reports to generate", cxxopts::value<u32>()->default_value("1024"))
      ("procs",       "Number of processes to spawn", cxxopts::value<u32>()->default_value("1"))
      ("no_save_prog",   "Do not save the generated prog before loading", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
      ("verify_crash",  "Verify the crash discovered", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
      ("load_prog",     "Load the prog from the file", cxxopts::value<std::string>()->default_value(""))
      ("de_oracle",   "De-oracleize the prog before loading", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
      ("v,verbose",   "Enable verbose output", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
      ("h,help",      "Print help message", cxxopts::value<bool>()->default_value("false")->implicit_value("true"));

    GlobalOptions::AddFuncOpts(options);

    cxxopts::ParseResult args;
    try {
      args = options.parse(argc, argv);
    } catch (cxxopts::exceptions::exception &e) {
      std::cerr << "Error: " << e.what() << std::endl;
      exit(1);
    }

    if (args.count("help")) {
      std::cout << options.help() << std::endl;
      exit(0);
    }

    std::string output;
    if (!args.count("output")) {
      std::cerr << "Error: The output directory (--output) is not given." << std::endl;
      exit(1);
    } else {
      output = args["output"].as<std::string>();
    }

    if (const int seed = args["seed"].as<int>(); seed >= 0) {
      Random::Get().Seed(seed);
    }

    u32 num_procs = args["procs"].as<u32>();

    GlobalOptions::Get().HandleFuncArgs(args);

    return {
        .uuid = "prog_" + std::to_string(getpid()),
        .sno = "generated",
        .output = output,
        .max_report = args["max_report"].as<u32>(),
        .report_num = 0,
        .verbose = args["verbose"].as<bool>(),
        .num_procs = num_procs,
        .proc_id = 0,
        .no_save_prog = args["no_save_prog"].as<bool>(),
        .verify_crash = args["verify_crash"].as<bool>(),
        .load_prog = args["load_prog"].as<std::string>(),
        .de_oracle = args["de_oracle"].as<bool>(),
    };
  }
};

// We don't spawn multi threads right now, but spawn multi processes.
// If this changed, make this local.
const size_t log_buf_size = 1024 * 1024;
static char log_buf[log_buf_size];

void gen_prog(const eBPFTestOpts &opts, vector<struct bpf_insn> &prog);

// Fork-server for program generation with timeout
class GenForkServer {
private:
  int pipe_fd[2]; // [0] for reading, [1] for writing
  pid_t child_pid;
  bool is_parent;

public:
  GenForkServer() : child_pid(-1), is_parent(false) {
    if (pipe(pipe_fd) == -1) {
      throw std::runtime_error("Failed to create pipe: " + std::string(strerror(errno)));
    }
  }

  ~GenForkServer() {
    close_read();
    close_write();
    kill_child();
  }

  void close_write() {
    if (pipe_fd[1] <= 0)
      return;
    close(pipe_fd[1]);
    pipe_fd[1] = -1;
  }

  void close_read() {
    if (pipe_fd[0] <= 0)
      return;
    close(pipe_fd[0]);
    pipe_fd[0] = -1;
  }

  void kill_child() {
    if (child_pid <= 0 || !is_parent)
      return;
    kill(child_pid, SIGKILL);
    waitpid(child_pid, nullptr, 0);
    child_pid = -1;
  }

  bool generate_program_with_timeout(
      const eBPFTestOpts &opts, vector<struct bpf_insn> &prog, u32 timeout_seconds
  ) {
    child_pid = fork();

    if (child_pid == -1) {
      throw std::runtime_error("Failed to fork: " + std::string(strerror(errno)));
    }

    if (child_pid == 0) {
      // Child process - generate program
      is_parent = false;
      close_read();

      try {
        gen_prog(opts, prog);

        // Write program size and data
        u32 prog_size = prog.size();
        write(pipe_fd[1], &prog_size, sizeof(prog_size));
        write(pipe_fd[1], prog.data(), prog_size * sizeof(struct bpf_insn));

        close(pipe_fd[1]);
        exit(0);
      } catch (const std::exception &e) {
        // Write error indicator
        u32 error_size = 0;
        write(pipe_fd[1], &error_size, sizeof(error_size));
        close(pipe_fd[1]);
        exit(1);
      }
    } else {
      // Parent process - wait with timeout
      is_parent = true;
      close_write();

      // Set pipe to non-blocking
      int flags = fcntl(pipe_fd[0], F_GETFL, 0);
      fcntl(pipe_fd[0], F_SETFL, flags | O_NONBLOCK);

      auto start_time = std::chrono::steady_clock::now();
      bool success = false;
      while (true) {
        // Check timeout
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        if (elapsed > std::chrono::seconds(timeout_seconds)) {
          cout << "[!] Generation timeout after " << timeout_seconds << " seconds" << endl;
          kill_child();
          return false;
        }

        // Check if child is still running
        int status;
        pid_t result = waitpid(child_pid, &status, WNOHANG);
        if (result == child_pid) {
          if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            success = true;
          }
          break;
        } else if (result == -1) {
          cout << "[!] Error waiting for child process" << endl;
          return false;
        }

        // Try to read from pipe
        u32 prog_size;
        ssize_t bytes_read = read(pipe_fd[0], &prog_size, sizeof(prog_size));
        if (bytes_read == sizeof(prog_size)) {
          if (prog_size > 0) {
            prog.resize(prog_size);
            bytes_read = read(pipe_fd[0], prog.data(), prog_size * sizeof(struct bpf_insn));
            if (bytes_read == prog_size * sizeof(struct bpf_insn)) {
              success = true;
              break;
            }
          }
          break;
        }

        // Sleep briefly before next check
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }

      close_read();
      return success;
    }
  }
};

// Wrapper function for generation with timeout
bool gen_prog_with_timeout(
    const eBPFTestOpts &opts, vector<struct bpf_insn> &prog, u32 timeout_seconds
) {
  GenForkServer server;
  return server.generate_program_with_timeout(opts, prog, timeout_seconds);
}

int load_prog(
    vector<struct bpf_insn> &prog, u32 log_level = 1, u32 prog_flags = BPF_F_TEST_REG_INVARIANTS
) {
  const char license[] = "GPL";
  union bpf_attr attr;

  memset(&attr, 0, sizeof(attr));
  attr.prog_type = BPF_PROG_TYPE_RAW_TRACEPOINT;
  attr.insns = reinterpret_cast<__u64>(prog.data());
  attr.insn_cnt = prog.size();
  attr.license = reinterpret_cast<__u64>(license);
  attr.log_level = log_level;
  attr.log_buf = reinterpret_cast<__u64>(log_buf);
  attr.log_size = log_buf_size;
  attr.prog_flags = prog_flags;

  return syscall(SYS_bpf, BPF_PROG_LOAD, &attr, sizeof(attr));
}

int test_run_prog(int prog_fd) {
  u64 ctx_in[12];
  union bpf_attr attr;

  memset(&attr, 0, sizeof(attr));
  attr.test.prog_fd = prog_fd;
  attr.test.ctx_in = reinterpret_cast<__u64>(ctx_in);
  attr.test.ctx_size_in = sizeof(ctx_in);

  int ret = syscall(SYS_bpf, BPF_PROG_TEST_RUN, &attr, sizeof(attr));
  if (ret < 0) {
    cerr << "[-] Failed to run the program: " << strerror(errno) << endl;
    exit(1);
  }
  return attr.test.retval;
}

void gen_prog(const eBPFTestOpts &opts, vector<struct bpf_insn> &prog) {
  const u32 MAX_PARAMS = 4;
  const u32 MAX_LOCALS = 2;

again:
  FunPlus fun(
      GetFunctionName(opts.uuid, opts.sno), MAX_PARAMS, MAX_LOCALS,
      GlobalOptions::Get().NumBblsPerFun, GlobalOptions::Get().MaxNumLoopsPerFun,
      GlobalOptions::Get().MaxNumBblsPerLoop
  );
  fun.Generate(false);

  // Set Z3 timeout to 3 seconds to prevent hanging
  z3::set_param("timeout", 3000);

  std::unique_ptr<UBFreeExec> exec = nullptr;
  for (int tries = 0; tries < GlobalOptions::Get().MaxNumExecsPerFun; ++tries) {
    vector<int> execPath = fun.SampleExec(
        GlobalOptions::Get().MaxNumExecStepsPerFun, GlobalOptions::Get().EnableConsistentExecs
    );
    exec = std::make_unique<UBFreeExec>(fun, execPath);
    int numSolved = exec->Solve(
        GlobalOptions::Get().NumInitsPerExec, GlobalOptions::Get().EnableInterestInits,
        GlobalOptions::Get().EnableRandomInits, GlobalOptions::Get().EnableInterestCoefs,
        /*debug=*/opts.verbose
    );
    if (numSolved != 0) {
      break;
    }
    exec = nullptr;
  }

  // We are unable to find any available executable executions
  if (exec == nullptr) {
    std::cerr << "[-] Unable to obtain any UB-free solutions within "
              << GlobalOptions::Get().MaxNumExecsPerFun << " execution samples" << std::endl;
    // TODO Confirm if this is necessary
    goto again;
  }

  fun.GenerateFuneBPFCode(*exec, &prog);
  cout << "[+] Prog size: " << prog.size() << endl;
}

void save_prog(eBPFTestOpts &opts, vector<struct bpf_insn> &prog_buf, const string &name) {
  string path = opts.output + "/" + name;
  std::ofstream ebpfFile(path, std::ios::binary);
  ebpfFile.write(
      reinterpret_cast<const char *>(prog_buf.data()), prog_buf.size() * sizeof(struct bpf_insn)
  );
  ebpfFile.close();
}

void save_report_prog(eBPFTestOpts &opts, vector<struct bpf_insn> &prog_buf, const string &prefix) {

  if (opts.report_num > opts.max_report) {
    cerr << "[+] Report suppressed" << endl;
    return;
  }

  string prog_name = prefix + std::to_string(opts.report_num) + ".bpf";
  save_prog(opts, prog_buf, prog_name);

  string vlog_name =
      opts.output + "/" + prefix + "_vlog" + std::to_string(opts.report_num) + ".txt";
  std::ofstream vlogFile(vlog_name);
  vlogFile.write(log_buf, strlen(log_buf));
  vlogFile.close();

  cout << "[+] Report saved: " << prog_name << " " << vlog_name << endl;
  opts.report_num++;
}

string get_gen_prog_name() { return "gen.bpf"; }

void save_gen_prog(eBPFTestOpts &opts, vector<struct bpf_insn> &prog_buf) {
  save_prog(opts, prog_buf, get_gen_prog_name());
}

// Analyze the rejection reason.
//
// The verifier log follows the format:
//   nth: insn		; reg_state
//   ...
//   rejection reason
//   processed .. insns (limit 1000000) max_states_per_insn xx total_states xx peak_states xx ...
//
// Where nth indicates the intruction index, insn is the instruction analyzed, and reg_state is
// the abstract state after analyzing insn, e.g., R8_w=Scalar(umax=..,umin=...);
// Then, the rejection reason follows, e.g., "frame pointer is read only" and "unreachable insn";
// Finally, the analysis summary follows.
//
// What we are currently interested is the rejection reason and the summary;
// For different reasons, we have different post processes;
// The summary is a great information for the test case.
void extract_vlog(char *vlog, string &summary, string &reason) {
  size_t len = std::strlen(vlog);
  Assert(len > 0, "Empty vlog");

  const char *p = vlog + len;
  int newline_count = 0;
  int line_length = 0;

  while (p != vlog) {
    --p;
    if (*p == '\n') {
      if (line_length == 0)
        continue;

      newline_count++;
      if (newline_count == 1) {
        summary = std::string(p + 1, line_length);
      } else if (newline_count == 2) {
        reason = std::string(p + 1, line_length);
        line_length = 0;
        break;
      } else {
        break;
      }
      line_length = 0;
      continue;
    }
    line_length++;
  }

  if (line_length > 0 && newline_count == 1)
    reason = std::string(p, line_length);

  Assert(summary.size() > 0, "Failed to locate the summary");
  Assert(reason.size() > 0, "Failed to locate the reason");

  summary.erase(summary.find_last_not_of(" \n\r\t") + 1);
  reason.erase(reason.find_last_not_of(" \n\r\t") + 1);
  return;
}

bool fp_interesting(const string &reason) {
  // The error reason "sequence of 8193 jumps is too complex" indicates the prog
  // contains too many jumps, which is not interesting
  static const std::vector<std::string> filters = {
      "The sequence of 8193 jumps is too complex", "BPF program is too large",
      "old state:", /* This is actually "infinite loop detected at insn ..", but it appears early
                     * (not just the last two entries), and the extract_vlog() does not handle it
                     * well currently.
                     */
  };
  return std::none_of(filters.begin(), filters.end(), [&](const std::string &f) {
    return reason.find(f) != std::string::npos;
  });
}

const u32 EXEC_MAGIC = 0xdeadbeef;
const struct bpf_insn RETURN_MAGIC_INSN = BPF_MOV32_IMM(BPF_REG_0, EXEC_MAGIC);
const struct bpf_insn VERIFIER_SINK_INSN = BPF_MOV32_IMM(BPF_REG_10, 0);

bool replace_oracle(vector<struct bpf_insn> &prog_buf, struct bpf_insn new_insn) {
  bool found = false;
  for (auto &insn: prog_buf) {
    if (memcmp(&insn, &VERIFIER_SINK_INSN, sizeof(struct bpf_insn)) == 0) {
      insn = new_insn;
      found = true;
      break;
    }
  }
  return found;
}

void test_one(eBPFTestOpts &opts, vector<struct bpf_insn> &prog_buf) {

  prog_buf.clear();

  // Use fork-server with timeout for generation
  if (!gen_prog_with_timeout(opts, prog_buf, 15)) { // 15 second timeout
    cout << "[!] Generation failed or timed out, skipping this iteration" << endl;
    return; // Skip to next iteration
  }

  // In case the kernel crashes, during loading the prog
  if (!opts.no_save_prog) {
    save_gen_prog(opts, prog_buf);
  }

  cout << "[*] Loading prog..." << endl;
  int prog_fd = load_prog(prog_buf);

  if (prog_fd >= 0) {
    // With the oracle embedded, we expect the prog to be rejected;
    // Accept effectively indicates a verifier bug.
    cout << "[!!] INTERESTING FINDING: Verifier BUG: oracle prog accepted" << endl;
    cout << "[!!] Prog size: " << prog_buf.size() << endl;
    save_report_prog(opts, prog_buf, "fn_oracle");
    close(prog_fd);
    return;
  }

  // if the reason is "frame pointer is read only", it's due to the oracle;
  // if the reason is "unreachable insn", it's due to the generator;
  // otherwise, it's a verifier false positive: we the prog is expected to
  // only be rejected due to the oracle.
  string summary, reason;
  extract_vlog(log_buf, summary, reason);

  if (reason.find("frame pointer is read only") != string::npos) {
    cout << "[+] \tResult: Rejected due to the oracle" << endl;
  } else if (reason.find("unreachable insn") != string::npos) {
    // unreachable blocks generated, fix the generator.
    cout << "[!] \tWARNING: " << reason << endl;
    return;
  } else {
    if (fp_interesting(reason)) {
      cout << "[!!] \tResult: Verifier false positive: " << reason << endl;
      save_report_prog(opts, prog_buf, "fp_oracle");
    }
    /* We cannot continue further: if we remove the oracle, then the prog
     * would still be rejected due to the same error.
     */
    return;
  }

  cout << "[*] \tSummary: " << summary << endl;

  // Replace the oracle with a return instruction, load and run the prog
  if (!replace_oracle(prog_buf, RETURN_MAGIC_INSN)) {
    cerr << "[!] WARNING: Oracle not found" << endl;
    return;
  }

  prog_fd = load_prog(prog_buf);
  if (prog_fd < 0) {
    extract_vlog(log_buf, summary, reason);
    if (fp_interesting(reason)) {
      cout << "[!!] \tResult: Verifier false positive: " << reason << endl;
      save_report_prog(opts, prog_buf, "fp_de-oracle");
    }
    return;
  }

  int ret = test_run_prog(prog_fd);
  if (ret != EXEC_MAGIC) {
    cout << "[!] \tWARNING: De-oracleized prog failed to run: " << ret << endl;
    save_report_prog(opts, prog_buf, "fp_de-oracle");
  }

  cout << "[*] \tde-oracleized prog executed successfully" << endl;
  close(prog_fd);
}

void run_worker(eBPFTestOpts opts) {
  std::filesystem::path outputDirectory = opts.output;
  std::filesystem::create_directories(outputDirectory);
  z3::set_param("parallel.enable", true);

  if (opts.verbose) {
    std::filesystem::create_directories(GetLoggingsDir(outputDirectory));
    Log::Get().SetFout(GetGenLogPath(opts.uuid, "log", outputDirectory, /*devnull=*/false));
  } else {
    Log::Get().SetFout(GetGenLogPath(opts.uuid, "log", outputDirectory, /*devnull=*/true));
  }

  cout << "[+] Worker " << opts.proc_id << " started (PID: " << getpid() << ")" << endl;

  vector<struct bpf_insn> prog_buf;
  double prog_per_sec = 0;
  u64 iterations = 0;
  u64 last_iter = 0;

  auto iter_start = std::chrono::high_resolution_clock::now();
  while (true) {
    cout << "[*] Worker " << opts.proc_id << " - Iterations: #" << iterations << endl;

    test_one(opts, prog_buf);

    if (iterations && iterations % 256 == 0) {
      auto end = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::minutes>(end - iter_start);
      if (duration.count() == 0) {
        iterations++;
        continue;
      }
      prog_per_sec = (double) (iterations - last_iter) / duration.count();
      cout << "[+] Worker " << opts.proc_id << " - Iterations: " << iterations
           << "\tProgs/min: " << prog_per_sec << endl;
      last_iter = iterations;
      iter_start = end;
    }
    iterations++;
    cout << "--------------------------------" << endl;
  }
}

void run_watcher(const eBPFTestOpts &opts) {
  cout << "[+] Spawning " << opts.num_procs << " worker processes..." << endl;

  // top level output directory
  std::filesystem::create_directories(opts.output);

  vector<pid_t> child_pids;
  for (u32 i = 1; i <= opts.num_procs; ++i) {
    pid_t pid = fork();

    if (pid == 0) {
      eBPFTestOpts worker_opts = opts;
      worker_opts.proc_id = i;
      worker_opts.output = opts.output + "/proc_" + std::to_string(i);
      run_worker(worker_opts);
      exit(0);
    } else if (pid > 0) {
      child_pids.push_back(pid);
      cout << "[+] Spawned worker " << i << " (PID: " << pid << ")" << endl;
    } else {
      cerr << "[-] Failed to spawn worker " << i << endl;
    }
  }

  cout << "[+] All workers spawned. Monitoring..." << endl;

  while (!child_pids.empty()) {
    int status;
    pid_t pid = wait(&status);

    if (pid > 0) {
      auto it = std::find(child_pids.begin(), child_pids.end(), pid);
      if (it != child_pids.end()) {
        u32 worker_id = std::distance(child_pids.begin(), it) + 1;
        if (WIFEXITED(status)) {
          cout << "[+] Worker " << worker_id << " (PID: " << pid << ") exited with code "
               << WEXITSTATUS(status) << endl;
        } else if (WIFSIGNALED(status)) {
          cout << "[!] Worker " << worker_id << " (PID: " << pid << ") killed by signal "
               << WTERMSIG(status) << endl;
        }
        child_pids.erase(it);
      }
    }
  }

  cout << "[+] All workers finished." << endl;
}

void load_prog_from_file(
    const eBPFTestOpts &opts, const string &path, vector<struct bpf_insn> &prog_buf
) {
  std::ifstream prog_file(path, std::ios::binary);
  auto prog_size = std::filesystem::file_size(path); // in bytes
  prog_buf.resize(prog_size / sizeof(struct bpf_insn));
  prog_file.read(reinterpret_cast<char *>(prog_buf.data()), prog_size);
  prog_file.close();
}

void do_verify(const eBPFTestOpts &opts, const string &dir) {
  cout << "[+] Verifying " << dir << endl;

  if (!std::filesystem::exists(dir)) {
    cerr << "[-] Directory does not exist: " << dir << endl;
    return;
  }

  // Check if the `gen.bpf` file exists
  string gen_prog_name = dir + "/" + get_gen_prog_name();
  if (!std::filesystem::exists(gen_prog_name)) {
    cerr << "[-] " << gen_prog_name << " does not exist" << endl;
    return;
  }

  // Load the prog
  vector<struct bpf_insn> prog_buf;
  load_prog_from_file(opts, gen_prog_name, prog_buf);

  cout << "[+] Loading the prog: " << gen_prog_name << " (" << prog_buf.size() << " insns)" << endl;

  int prog_fd = load_prog(prog_buf, 2);
  string summary, reason;
  extract_vlog(log_buf, summary, reason);
  cout << "[+] \tSummary: " << summary << endl;
  cout << "[+] \tReason: " << reason << endl;
  if (prog_fd >= 0) {
    int ret = test_run_prog(prog_fd);
    cout << "[*] \tReturn value: " << ret << endl;
    close(prog_fd);
  }
  return;
}

void verify_crash(const eBPFTestOpts &opts) {
  // Check which generated prog caused the kernel crash
  // If proc is one, then only checks the `output` directory
  // Otherwise, checks the `output/proc_<id>` directory
  if (opts.num_procs == 1) {
    do_verify(opts, opts.output);
  } else {
    for (u32 i = 1; i <= opts.num_procs; ++i) {
      do_verify(opts, opts.output + "/proc_" + std::to_string(i));
    }
  }
}

void load_prog_only(const eBPFTestOpts &opts) {
  cout << "[+] Loading the prog: " << opts.load_prog << "..." << endl;
  vector<struct bpf_insn> prog_buf;
  load_prog_from_file(opts, opts.load_prog, prog_buf);

  if (opts.de_oracle) {
    if (!replace_oracle(prog_buf, RETURN_MAGIC_INSN)) {
      cout << "[!] \tWARNING: Oracle not found, skipping de-oracleization" << endl;
    } else {
      cout << "[+] \tDe-oracleized the prog" << endl;
    }
  }

  // Load the prog
  int prog_fd = load_prog(prog_buf, 2, BPF_F_TEST_REG_INVARIANTS);
  cout << "--------------------------------" << endl;
  cout << log_buf;
  cout << "--------------------------------" << endl;

  if (prog_fd < 0)
    return;

  int ret = test_run_prog(prog_fd);
  cout << "[*] \tProg executed, return value: 0x" << std::hex << ret << std::dec << endl;
  if (opts.de_oracle && ret != EXEC_MAGIC) {
    cout << "[!] \tWARNING: De-oracleized prog failed to run, expected magic: " << EXEC_MAGIC
         << endl;
  }

  close(prog_fd);
  return;
}

int main(int argc, char **argv) {
  auto cliOpts = eBPFTestOpts::Parse(argc, argv);

  if (cliOpts.verify_crash) {
    cout << "[+] Verifying the errors discovered..." << endl;
    verify_crash(cliOpts);
    return 0;
  }

  if (!cliOpts.load_prog.empty()) {
    load_prog_only(cliOpts);
    return 0;
  }

  if (cliOpts.num_procs == 1) {
    run_worker(cliOpts);
  } else {
    run_watcher(cliOpts);
  }

  return 0;
}
