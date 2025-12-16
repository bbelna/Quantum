param(
    [switch]$Run,   # enables --run or -Run or -r
    [switch]$Tests  # enables kernel tests via KERNEL_TESTS
)

# Paths
$ProjectPathWSL = "/mnt/d/Development/Git/Quantum"
$ImageWinPath   = "D:\Development\Git\Quantum\Build\Quantum.img"

Write-Host "=== Building Quantum (WSL) ==="

# Build inside WSL
$makeCmd = "cd $ProjectPathWSL && "
if ($Tests) {
    $makeCmd += "export EXTRA_CFLAGS32='-DKERNEL_TESTS' && "
}
$makeCmd += "make clean && make all"

wsl -e bash -lc $makeCmd
if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed. Aborting."
    exit 1
}

Write-Host "`n[OK] Build completed."

# Optional run
if ($Run) {
    .\Debug
}
else {
    Write-Host "`nUse --run to automatically start QEMU."
}
