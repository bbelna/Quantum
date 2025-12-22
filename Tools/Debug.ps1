Write-Host "`n=== Launching QEMU (Windows) ==="

qemu-system-i386 `
    -fda ../Build/Quantum.img `
    -m 64M `
    -monitor none `
    -serial none `
    -parallel none `
    -debugcon stdio `
    -no-reboot `
    -no-shutdown `

Write-Host "`nQEMU exited."
