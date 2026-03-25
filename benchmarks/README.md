# Benchmarking Rig

## Measurement Environment

| | |
|---|---|
| Host machine | Apple M2 MacBook Air 13" |
| Virtualization | UTM (QEMU, Apple Silicon) |
| Guest OS | Ubuntu 22.04 LTS |
| Guest resources | 4 vCPUs, 4GB RAM |
| Compiler | GCC 11.4, -O2 |
| Load generator | wrk 4.2.0 |

*Note: wrk embeds LuaJIT for scripted workloads — Lua does not need to be installed separately.*

To recreate this environment, set up a new Ubuntu 22.04 LTS VM in UTM with 4 vCPUs and 4GB RAM. Install dependencies:

```bash
sudo apt update
sudo apt install -y gcc make build-essential libssl-dev git
```

Install wrk:

```bash
sudo apt install wrk
```

If not available via apt, build from source:

```bash
git clone https://github.com/wg/wrk.git
cd wrk && make
sudo cp wrk /usr/local/bin
```

Create the test files for the uniform small and uniform large workloads:

```bash
mkdir -p www/test
dd if=/dev/zero of=www/test/small.bin bs=1K count=4
dd if=/dev/zero of=www/test/large.bin bs=1M count=1
```

Create the heavy-tailed workload by generating this Lua script used by wrk to mix request types:

```bash
cat > benchmarks/heavy_tail.lua << 'EOF'
math.randomseed(os.time())

request = function()
    local r = math.random()
    if r < 0.90 then
        return wrk.format("GET", "/small.bin")
    else
        return wrk.format("GET", "/large.bin")
    end
end
EOF
```

---

## Measurements and Output

### Independent Variables

- **Concurrency levels:** 1, 10, 25, 50, 100, 200 concurrent connections
- **File size workloads:**
  - Uniform small — all requests for a 4KB file
  - Uniform large — all requests for a 1MB file
  - Heavy-tailed — 90% small / 10% large requests by count

### Dependent Variables

- **Throughput** — requests per second
- **Response time** — mean, p50, p95, p99 (milliseconds)
- **Error rate** — connection resets and timeouts

### Protocol

Each configuration is run for 30 seconds with a 5-second warmup period excluded from measurement. Five independent trials are collected per configuration, reporting the mean and standard deviation across trials. The server is restarted between trials to clear any warm state. Thread pool size is fixed at 4 and buffer capacity is fixed at 16 across all runs.

### Output

Raw results are saved to `./raw/`. Each file is named by workload, concurrency level, and trial number — for example, `small_c50_t2.txt` is the second trial of the uniform small workload at concurrency 50.

---

## Running the Benchmarks

Build the server:

```bash
make
```

From the project root, run the benchmark:

```bash
chmod +x benchmark.sh
./benchmark.sh
```

*Note: A full run takes approximately 50–55 minutes (75 configurations × 5 trials × ~35 seconds each plus server restart and sleep overhead).*