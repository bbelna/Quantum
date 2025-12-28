param(
    [switch]$Run,   # enables --run or -Run or -r
    [switch]$Tests, # enables kernel tests via KERNEL_TESTS
    [Alias('c')]
    [switch]$Clean  # enables make clean before build
)

# Paths
$ProjectPath = "D:\Development\Git\Quantum"
$ProjectPathWSL = "/mnt/d/Development/Git/Quantum"
$ImageWinPath   = $ProjectPath + "\Build\Quantum.img"
$ImageWslPath   = "$ProjectPathWSL/Build/Quantum.img"
$ManifestPath   = Join-Path $ProjectPath "Configuration\InitManifest.json"

Write-Host "=== Building Quantum (WSL) ==="

# Build inside WSL
$makeCmd = "cd $ProjectPathWSL && "

if ($Tests) {
    $makeCmd += "export EXTRA_CFLAGS32='-DKERNEL_TESTS' && "
}

if ($Clean) {
    $makeCmd += "make clean && "
}

$makeCmd += "make all"

wsl -e bash -lc $makeCmd
if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed. Aborting."
    exit 1
}

Write-Host "`n[OK] Build completed."

# Optional run
if ($Run) {
    $ToolsPath = Join-Path $ProjectPath "Tools"

    $originalLocation = Get-Location
    $switched = $false

    try {
        if ($originalLocation.Path -ne $ToolsPath) {
            Push-Location $ToolsPath
            $switched = $true
        }

        .\Debug
    }
    finally {
        if ($switched) {
            Pop-Location
        }
    }
}
else {
    Write-Host "`nUse --run to automatically start QEMU."
}
