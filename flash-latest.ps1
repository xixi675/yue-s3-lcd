param(
    [string]$Port = "COM7"
)

$ErrorActionPreference = "Stop"
$repo = "xixi675/yue-s3-lcd"
$releaseApi = "https://api.github.com/repos/$repo/releases/latest"
$output = Join-Path $env:TEMP "yue-s3-lcd-latest-merged.bin"

Write-Host "Finding latest yue-s3-lcd release..."
$release = Invoke-RestMethod -Uri $releaseApi -Headers @{
    "User-Agent" = "yue-s3-lcd-flasher"
}

$asset = $release.assets |
    Where-Object { $_.name -like "*-merged.bin" } |
    Select-Object -First 1

if (-not $asset) {
    throw "No merged firmware asset found in the latest GitHub release."
}

Write-Host "Downloading $($asset.name)..."
Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $output

Write-Host "Flashing $Port..."
python -m esptool --chip esp32s3 -p $Port -b 460800 `
    --before default-reset --after hard-reset `
    write-flash 0x0 $output

Write-Host "Firmware flashed successfully."
