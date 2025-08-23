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

  static eBPFTestOpts Parse(int argc, char **argv) {
    cxxopts::Options options("bpf_test");
    options.add_options()
      ("o,output",    "Directory to save the testing eBPF program", cxxopts::value<std::string>())
      ("s,seed",      "Seed for random sampling (negative for true random)", cxxopts::value<int>()->default_value("-1"))
      ("max_report",  "Maximum number of reports to generate", cxxopts::value<u32>()->default_value("1024"))
      ("procs",       "Number of processes to spawn", cxxopts::value<u32>()->default_value("1"))
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

int load_prog(vector<struct bpf_insn> &prog) {
  const char license[] = "GPL";
  union bpf_attr attr;

  memset(&attr, 0, sizeof(attr));
  attr.prog_type = BPF_PROG_TYPE_RAW_TRACEPOINT;
  attr.insns = reinterpret_cast<__u64>(prog.data());
  attr.insn_cnt = prog.size();
  attr.license = reinterpret_cast<__u64>(license);
  attr.log_level = 1;
  attr.log_buf = reinterpret_cast<__u64>(log_buf);
  attr.log_size = log_buf_size;

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

void save_prog(eBPFTestOpts &opts, vector<struct bpf_insn> &prog_buf) {
  if (opts.report_num > opts.max_report) {
    cerr << "[+] Report suppressed" << endl;
    return;
  }

  string prog_name = opts.output + "/fn" + std::to_string(opts.report_num) + ".bpf";
  std::ofstream ebpfFile(prog_name, std::ios::binary);
  ebpfFile.write(
      reinterpret_cast<const char *>(prog_buf.data()), prog_buf.size() * sizeof(struct bpf_insn)
  );
  ebpfFile.close();

  string vlog_name = opts.output + "/fn_vlog" + std::to_string(opts.report_num) + ".txt";
  std::ofstream vlogFile(vlog_name);
  vlogFile.write(log_buf, strlen(log_buf));
  vlogFile.close();

  cout << "[+] Report saved: " << prog_name << " " << vlog_name << endl;
  opts.report_num++;
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

void test_one(eBPFTestOpts &opts, vector<struct bpf_insn> &prog_buf) {

  prog_buf.clear();

  // Use fork-server with timeout for generation
  if (!gen_prog_with_timeout(opts, prog_buf, 15)) { // 15 second timeout
    cout << "[!] Generation failed or timed out, skipping this iteration" << endl;
    return; // Skip to next iteration
  }

  cout << "[*] Loading prog..." << endl;
  int prog_fd = load_prog(prog_buf);

  if (prog_fd >= 0) {
    // With the oracle embedded, we expect the prog to be rejected;
    // Accept effectively indicates a verifier bug.
    cout << "[!!] INTERESTING FINDING: Verifier BUG: oracle prog accepted" << endl;
    cout << "[!!] Prog size: " << prog_buf.size() << endl;
    save_prog(opts, prog_buf);
    close(prog_fd);
    return;
  }

  string summary, reason;
  extract_vlog(log_buf, summary, reason);

  // if the reason is "frame pointer is read only", it's due to the oracle;
  // if the reason is "unreachable insn", it's due to the generator;
  // otherwise, it's a verifier false positive: we the prog is expected to
  // only be rejected due to the oracle.

  if (reason.find("frame pointer is read only") != string::npos) {
    cout << "[+] \tResult: Rejected due to the oracle" << endl;
  } else if (reason.find("unreachable insn") != string::npos) {
    cout << "[!] \tWARNING: " << reason << endl;
    return; // can't fix or execute further.
  } else {
    cout << "[!!] \tResult: Verifier false positive: " << reason << endl
         << "\tSummary: " << summary << endl;
    save_prog(opts, prog_buf);
    return;
  }

  cout << "[*] \tSummary: " << summary << endl;

  // Remove the oracle, load and run the prog
  bool found = false;
  struct bpf_insn guilty = BPF_MOV32_IMM(BPF_REG_10, 0);
  for (auto &insn: prog_buf) {
    if (memcmp(&insn, &guilty, sizeof(struct bpf_insn)) == 0) {
      insn = BPF_MOV32_IMM(BPF_REG_0, 0);
      found = true;
      break;
    }
  }
  if (!found) {
    cerr << "[!] WARNING: Oracle not found" << endl;
    return;
  }

  const int magic = 0xdeadbeef;
  prog_buf[prog_buf.size() - 2] = BPF_MOV32_IMM(BPF_REG_0, magic);
  prog_fd = load_prog(prog_buf);
  if (prog_fd < 0) {
    cerr << "[-] \tWARNING: Failed to load the de-oracleized program" << endl;
    extract_vlog(log_buf, summary, reason);
    cout << "[*] \tReason: " << reason << endl;
    cout << "[*] \tSummary: " << summary << endl;
    return;
  }

  int ret = test_run_prog(prog_fd);
  if (ret != magic) {
    cout << "[!] \tWARNING: De-oracleized prog failed to run: " << ret << endl;
    save_prog(opts, prog_buf);
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

int main(int argc, char **argv) {
  auto cliOpts = eBPFTestOpts::Parse(argc, argv);

  if (cliOpts.num_procs == 1) {
    run_worker(cliOpts);
  } else {
    run_watcher(cliOpts);
  }

  return 0;
}
