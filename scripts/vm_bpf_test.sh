#!/usr/bin/env bash

set -euo pipefail

# Default configuration
DEFAULT_MEM="64G"
DEFAULT_CPUS="32"
DEFAULT_SSH_PORT="10080"
DEFAULT_SSH_USER="root"
DEFAULT_SSH_KEY="imgs/bookworm.id_rsa"
DEFAULT_VM_IMG="imgs/bookworm.img"
DEFAULT_KERNEL_PATH="imgs/bzImage"
DEFAULT_VM_FSD=$(which virtiofsd)
DEFAULT_BPF_TEST_PATH="build/bin/bpf_test"
DEFAULT_OUTPUT_DIR="bpf_test_output"
DEFAULT_SHARED_DIR="output"
DEFAULT_PROCS="8"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

fatal() {
    log_error "$1"
    exit 1
}

trap 'fatal "Error on line $LINENO"' ERR

# Function to show usage
show_usage() {
    cat << EOF
Usage: $0 [OPTIONS] -- [BPF_TEST_ARGS...]

VM-based eBPF testing script that runs bpf_test inside a QEMU virtual machine.

OPTIONS:
    -h, --help              Show this help message
    -m, --memory MEM        VM memory size (default: $DEFAULT_MEM)
    -c, --cpus CPUS         Number of VM CPUs (default: $DEFAULT_CPUS)
    -p, --ssh-port PORT     SSH port for VM access (default: $DEFAULT_SSH_PORT)
    -u, --ssh-user USER     SSH user for VM access (default: $DEFAULT_SSH_USER)
    -k, --ssh-key PATH      SSH key for VM access (default: $DEFAULT_SSH_KEY)
    -i, --image PATH        VM disk image path (default: $DEFAULT_VM_IMG)
    -K, --kernel PATH       VM kernel path (default: $DEFAULT_KERNEL_PATH)
    -f, --fsd PATH          virtiofsd binary path (default: $DEFAULT_VM_FSD)
    -b, --bpf-test PATH     Path to bpf_test binary (default: $DEFAULT_BPF_TEST_PATH)
    -s, --shared DIR        Result directory shared with VM (default: $DEFAULT_SHARED_DIR)
    -o, --output DIR        Output directory for bpf_test in VM (default: $DEFAULT_OUTPUT_DIR)
    -t, --timeout SEC       Timeout for VM boot in seconds (default: 120)
    -n, --procs NUM         Number of processes to spawn: bpf_test --procs (default: $DEFAULT_PROCS)

BPF_TEST_ARGS:
    All arguments after -- are passed directly to bpf_test inside the VM.

EXAMPLES:
    # Basic usage with default settings
    $0

    # Custom VM configuration
    $0 -m 16G -c 8 -i /path/to/vm.img -K /path/to/kernel

    # With custom bpf_test binary
    $0 -b /path/to/bpf_test

EOF
}

# Function to wait for a condition
wait_for() {
    local what="$1" file="$2" max="$3"
    log_info "Waiting for $what..."
    for ((i=1; i<=max; i++)); do
        if [[ -e "$file" ]]; then
            log_success "$what is ready"
            return 0
        fi
        sleep 1
    done
    fatal "$what did not appear within ${max}s: $file"
}

# Function to execute command in VM via SSH
_vmcmd() {
    ssh -o ConnectTimeout=1 -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null \
        -p "$SSH_PORT" -i "$SSH_KEY" "$SSH_USER"@localhost "$@"
}

_vmcopy() {
    local src="$1" dst="$2"
    scp -o ConnectTimeout=5 -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null \
        -P "$SSH_PORT" -i "$SSH_KEY" "$src" "$SSH_USER@localhost:$dst"
}

_vmget() {
    local src="$1" dst="$2"
    scp -o ConnectTimeout=5 -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null \
        -P "$SSH_PORT" -i "$SSH_KEY" "$SSH_USER@localhost:$src" "$dst"
}

# Parse command line arguments
MEM="$DEFAULT_MEM"
CPUS="$DEFAULT_CPUS"
SSH_PORT="$DEFAULT_SSH_PORT"
SSH_USER="$DEFAULT_SSH_USER"
SSH_KEY="$DEFAULT_SSH_KEY"
VM_IMG="$DEFAULT_VM_IMG"
KERNEL_PATH="$DEFAULT_KERNEL_PATH"
VM_FSD="$DEFAULT_VM_FSD"
BPF_TEST_PATH="$DEFAULT_BPF_TEST_PATH"
OUTPUT_DIR="$DEFAULT_OUTPUT_DIR"
SHARED_DIR="$DEFAULT_SHARED_DIR"
PROCS="$DEFAULT_PROCS"
TIMEOUT=120
DEBUG=false

# Parse options
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_usage
            exit 0
            ;;
        -m|--memory)
            if [[ $# -lt 2 ]]; then fatal "Missing argument for $1"; fi
            MEM="$2"
            shift 2
            ;;
        -c|--cpus)
            if [[ $# -lt 2 ]]; then fatal "Missing argument for $1"; fi
            CPUS="$2"
            shift 2
            ;;
        -p|--ssh-port)
            if [[ $# -lt 2 ]]; then fatal "Missing argument for $1"; fi
            SSH_PORT="$2"
            shift 2
            ;;
        -u|--ssh-user)
            if [[ $# -lt 2 ]]; then fatal "Missing argument for $1"; fi
            SSH_USER="$2"
            shift 2
            ;;
        -k|--ssh-key)
            if [[ $# -lt 2 ]]; then fatal "Missing argument for $1"; fi
            SSH_KEY="$2"
            shift 2
            ;;
        -i|--image)
            if [[ $# -lt 2 ]]; then fatal "Missing argument for $1"; fi
            VM_IMG="$2"
            shift 2
            ;;
        -K|--kernel)
            if [[ $# -lt 2 ]]; then fatal "Missing argument for $1"; fi
            KERNEL_PATH="$2"
            shift 2
            ;;
        -f|--fsd)
            if [[ $# -lt 2 ]]; then fatal "Missing argument for $1"; fi
            VM_FSD="$2"
            shift 2
            ;;
        -b|--bpf-test)
            if [[ $# -lt 2 ]]; then fatal "Missing argument for $1"; fi
            BPF_TEST_PATH="$2"
            shift 2
            ;;
        -o|--output)
            if [[ $# -lt 2 ]]; then fatal "Missing argument for $1"; fi
            OUTPUT_DIR="$2"
            shift 2
            ;;
        -s|--shared)
            if [[ $# -lt 2 ]]; then fatal "Missing argument for $1"; fi
            SHARED_DIR="$2"
            shift 2
            ;;
        -t|--timeout)
            if [[ $# -lt 2 ]]; then fatal "Missing argument for $1"; fi
            TIMEOUT="$2"
            shift 2
            ;;
        -n|--procs)
            if [[ $# -lt 2 ]]; then fatal "Missing argument for $1"; fi
            PROCS="$2"
            shift 2
            ;;
        --debug)
            DEBUG=true
            shift
            ;;
        --)
            shift
            break
            ;;
        *)
            log_error "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

# Store bpf_test arguments
BPF_TEST_ARGS=("$@")

# Debug output
if [[ "$DEBUG" == true ]]; then
    log_info "Configuration:"
    log_info "  Memory: $MEM"
    log_info "  CPUs: $CPUS"
    log_info "  SSH Port: $SSH_PORT"
    log_info "  SSH User: $SSH_USER"
    log_info "  SSH Key: $SSH_KEY"
    log_info "  VM Image: $VM_IMG"
    log_info "  Kernel: $KERNEL_PATH"
    log_info "  virtiofsd: $VM_FSD"
    log_info "  bpf_test: $BPF_TEST_PATH"
    log_info "  Output Dir: $OUTPUT_DIR"
    log_info "  Shared Dir: $SHARED_DIR"
    log_info "  Timeout: $TIMEOUT"
    log_info "  Procs: $PROCS"
fi

# Check required files
require() {
    if [[ ! -f "$1" ]]; then
        fatal "$2 not found: $1"
    fi
}

# Validate required files
require "$VM_IMG" "VM image"
require "$KERNEL_PATH" "Kernel"
if [[ -z "$VM_FSD" ]]; then
    fatal "virtiofsd not found in PATH"
fi
require "$VM_FSD" "virtiofsd"
require "$SSH_KEY" "SSH key"
require "$BPF_TEST_PATH" "bpf_test binary"
command -v qemu-system-x86_64 >/dev/null 2>&1 || fatal "qemu-system-x86_64 not found in PATH"
command -v ssh >/dev/null 2>&1 || fatal "ssh not found in PATH"
command -v scp >/dev/null 2>&1 || fatal "scp not found in PATH"

mkdir -p "$SHARED_DIR"

RESULT_DIR="$SHARED_DIR"
SOCK="$RESULT_DIR/bpf-test.sock"
VIRTIOFSD_PIDFILE="$RESULT_DIR/virtiofsd.pid"
VIRTIOFSD_LOG="$RESULT_DIR/virtiofsd.log"
VM_PIDFILE="$RESULT_DIR/vm.pid"
VM_LOG="$RESULT_DIR/vm.log"
BPF_TEST_RESULTS="$RESULT_DIR/$OUTPUT_DIR"
VM_BPF_TEST_OUTPUT="$OUTPUT_DIR/bpf_test.log" # bpf_test output in VM
BPF_TEST_OUTPUT="$RESULT_DIR/bpf_test.log"    # bpf_test output in host

# Create results directory
mkdir -p "$BPF_TEST_RESULTS"
if [[ ! -d "$BPF_TEST_RESULTS" ]]; then
    fatal "Failed to create results directory: $BPF_TEST_RESULTS"
fi

# Clean up any existing files
rm -f "$SOCK" "$VM_PIDFILE" "$VIRTIOFSD_PIDFILE" "$VIRTIOFSD_LOG" "$VM_LOG"

log_info "Starting VM-based eBPF testing..."

# Start virtiofsd
log_info "Starting virtiofsd..."
"$VM_FSD" --socket-path "$SOCK" --shared-dir "$SHARED_DIR" > "$VIRTIOFSD_LOG" 2>&1 &
VIRTIOFSD_PID=$!
echo "$VIRTIOFSD_PID" > "$VIRTIOFSD_PIDFILE"

wait_for "virtiofsd socket" "$SOCK" 5
[[ -S "$SOCK" ]] || fatal "virtiofsd did not create socket $SOCK"

# Launch QEMU VM
log_info "Launching QEMU VM..."
qemu-system-x86_64 \
    -m "$MEM" \
    -smp "$CPUS" \
    -kernel "$KERNEL_PATH" \
    -append "console=ttyS0 root=/dev/sda earlyprintk=serial net.ifnames=0" \
    -drive file="$VM_IMG",format=raw \
    -net user,host=10.0.2.10,hostfwd=tcp:127.0.0.1:"$SSH_PORT"-:22 \
    -net nic,model=e1000 \
    -enable-kvm \
    -nographic \
    -pidfile "$VM_PIDFILE" \
    -object memory-backend-file,id=mem,size="$MEM",mem-path=/dev/shm,share=on \
    -numa node,memdev=mem \
    -chardev socket,id=char0,path="$SOCK" \
    -device vhost-user-fs-pci,queue-size=1024,chardev=char0,tag=bpf-test \
    -snapshot \
    > "$VM_LOG" 2>&1 &

wait_for "QEMU PID file" "$VM_PIDFILE" 10

# Wait for VM to boot
log_info "Waiting for VM to boot..."
VM_BOOTED=false
for ((i=1; i<=TIMEOUT; i++)); do
    echo -ne "\rWaiting for VM to boot $i/$TIMEOUT (s)..."
    if _vmcmd "pwd" > /dev/null 2>&1; then
        echo
        log_success "VM is ready. SSH available on port $SSH_PORT."
        VM_BOOTED=true
        break
    fi
    sleep 1
done

if [[ "$VM_BOOTED" != true ]]; then
    echo
    fatal "VM did not become available via SSH on port $SSH_PORT within ${TIMEOUT}s"
fi

# Mount shared directory in VM
log_info "Mounting shared directory..."
VM_WORK_DIR="/mnt/shared"
_vmcmd "mkdir -p $VM_WORK_DIR" || true
if ! _vmcmd "mount -t virtiofs bpf-test $VM_WORK_DIR"; then
    fatal "Failed to mount virtiofs at $VM_WORK_DIR"
fi
log_success "Shared directory mounted: $SHARED_DIR => $VM_WORK_DIR"

# Copy bpf_test binary to VM
log_info "Copying bpf_test binary to VM..."
VM_BPF_TEST_PATH="$VM_WORK_DIR/bpf_test"
if ! _vmcopy "$BPF_TEST_PATH" "$VM_BPF_TEST_PATH"; then
    fatal "Failed to copy bpf_test binary to VM"
fi
if ! _vmcmd "chmod +x $VM_BPF_TEST_PATH"; then
    fatal "Failed to chmod bpf_test binary in VM"
fi
log_success "bpf_test binary copied to VM: $VM_BPF_TEST_PATH"

# Create results directory in VM
VM_RESULTS_DIR="$VM_WORK_DIR/$OUTPUT_DIR"
if ! _vmcmd "mkdir -p $VM_RESULTS_DIR"; then
    fatal "Failed to create results directory in VM: $VM_RESULTS_DIR"
fi

# Execute bpf_test in VM
log_info "Executing bpf_test in VM..."
log_info "Command: $VM_BPF_TEST_PATH --output $VM_RESULTS_DIR --procs $PROCS ${BPF_TEST_ARGS[*]}"

# Run bpf_test in the background inside the VM and return immediately
# Use a wrapper script to properly handle background execution
_vmcmd "cat > $VM_WORK_DIR/run_bpf_test.sh << 'EOF'
#!/bin/bash
cd $VM_WORK_DIR
exec $VM_BPF_TEST_PATH --output $VM_RESULTS_DIR --procs $PROCS ${BPF_TEST_ARGS[*]} > $VM_BPF_TEST_OUTPUT 2>&1
EOF"

_vmcmd "chmod +x $VM_WORK_DIR/run_bpf_test.sh"

# Start the process in background using nohup and redirect to /dev/null to avoid SSH hanging
if _vmcmd "nohup $VM_WORK_DIR/run_bpf_test.sh > /dev/null 2>&1 & echo \$! > $VM_WORK_DIR/bpf_test.pid"; then
    BPF_TEST_PID=$(_vmcmd "cat $VM_WORK_DIR/bpf_test.pid")
    if [[ -n "$BPF_TEST_PID" ]]; then
        log_success "bpf_test started successfully in background (PID: $BPF_TEST_PID)"
        # Verify the process is actually running
        sleep 1
        if _vmcmd "kill -0 $BPF_TEST_PID 2>/dev/null"; then
            log_success "bpf_test process confirmed running"
        else
            log_warning "bpf_test process may have failed to start properly"
        fi
    else
        log_warning "Failed to get bpf_test PID"
    fi
else
    log_warning "Failed to start bpf_test in background (check $BPF_TEST_OUTPUT for details)"
fi

# Show summary
log_success "VM-based testing started."
log_info "Output files:"
log_info "  VM log: $VM_LOG"
log_info "  bpf_test output: $BPF_TEST_OUTPUT"
log_info "  VM PID: $(cat "$VM_PIDFILE" 2>/dev/null || echo 'unknown')"
if [[ -n "$BPF_TEST_PID" ]]; then
    log_info "  bpf_test PID: $BPF_TEST_PID"
fi
log_info "  SSH port: $SSH_PORT"
log_info "  To connect: ssh -p $SSH_PORT $SSH_USER@localhost -i $SSH_KEY"
log_info "  To check bpf_test status: ssh -p $SSH_PORT $SSH_USER@localhost -i $SSH_KEY 'ps -p $BPF_TEST_PID'"
log_info "  To view bpf_test output: ssh -p $SSH_PORT $SSH_USER@localhost -i $SSH_KEY 'tail -f $VM_BPF_TEST_OUTPUT'"
