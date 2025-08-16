param(
    [string]$SdkPath = ".\sdk",
    [string]$OutputPath = ".\decompiled_sdk",
    [string]$UnluacJar = ".\unluac_2023_12_24.jar"
)

# Check if required files exist
if (-not (Test-Path $SdkPath)) {
    Write-Error "SDK path '$SdkPath' not found!"
    exit 1
}

if (-not (Test-Path $UnluacJar)) {
    Write-Error "Unluac jar file '$UnluacJar' not found!"
    exit 1
}

# Create output directory if it doesn't exist
if (-not (Test-Path $OutputPath)) {
    New-Item -ItemType Directory -Path $OutputPath -Force | Out-Null
}

# Get all .lua files recursively from sdk folder
$luaFiles = Get-ChildItem -Path $SdkPath -Filter "*.lua" -Recurse

if ($luaFiles.Count -eq 0) {
    Write-Warning "No .lua files found in '$SdkPath'"
    exit 0
}

Write-Host "Found $($luaFiles.Count) Lua files to decompile..."

$successCount = 0
$failCount = 0

foreach ($file in $luaFiles) {
    # Calculate relative path from sdk root
    $relativePath = $file.FullName.Substring($SdkPath.Length).TrimStart('\')
    
    # Create corresponding output path
    $outputFile = Join-Path $OutputPath $relativePath
    $outputDir = Split-Path $outputFile -Parent
    
    # Create output directory if it doesn't exist
    if (-not (Test-Path $outputDir)) {
        New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
    }
    
    Write-Host "Decompiling: $relativePath"
    
    try {
        # Run unluac decompiler
        $result = java -jar $UnluacJar $file.FullName 2>&1
        
        if ($LASTEXITCODE -eq 0) {
            # Save decompiled content to output file
            $result | Out-File -FilePath $outputFile -Encoding UTF8
            $successCount++
            Write-Host "  Success: $outputFile" -ForegroundColor Green
        } else {
            $failCount++
            Write-Host "  Failed: $($file.Name) - $result" -ForegroundColor Red
        }
    } catch {
        $failCount++
        Write-Host "  Error: $($file.Name) - $($_.Exception.Message)" -ForegroundColor Red
    }
}

Write-Host "`nDecompilation completed!"
Write-Host "Success: $successCount files" -ForegroundColor Green
Write-Host "Failed: $failCount files" -ForegroundColor Red