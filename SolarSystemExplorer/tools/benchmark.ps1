[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$ExePath,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [switch]$Hidden
)

$ErrorActionPreference = 'Stop'
$executable = (Get-Item -LiteralPath $ExePath).FullName
$runName = 'benchmark-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff')
$runDirectory = New-Item -ItemType Directory -Path (Join-Path $OutputDirectory $runName) -Force
$windowMode = if ($Hidden) { 'Hidden' } else { 'Visible' }
$rows = foreach ($quality in 'high', 'low') {
    foreach ($bloom in 'on', 'off') {
        $options = @('--width', '1280', '--height', '720', '--benchmark', '--frames', '1800', '--quality', $quality)
        if ($bloom -eq 'off') { $options += '--no-bloom' }
        if ($Hidden) { $options += '--hidden' }
        $startInfo = New-Object System.Diagnostics.ProcessStartInfo
        $startInfo.FileName = $executable
        $startInfo.WorkingDirectory = [IO.Path]::GetDirectoryName($executable)
        $startInfo.Arguments = $options -join ' '
        $startInfo.UseShellExecute = $false
        $startInfo.CreateNoWindow = $true
        $startInfo.RedirectStandardOutput = $true
        $startInfo.RedirectStandardError = $true
        Write-Host "Measuring $quality quality, Bloom $bloom, $windowMode window"
        $process = [Diagnostics.Process]::Start($startInfo)
        try {
            $stdout = $process.StandardOutput.ReadToEndAsync()
            $stderr = $process.StandardError.ReadToEndAsync()
            $process.WaitForExit()
            $log = $stdout.GetAwaiter().GetResult() + [Environment]::NewLine + $stderr.GetAwaiter().GetResult()
            $exitCode = $process.ExitCode
        } finally {
            $process.Dispose()
        }
        $logPath = Join-Path $runDirectory.FullName "$quality-bloom-$bloom.log"
        [IO.File]::WriteAllText($logPath, $log)
        $summary = [regex]::Match($log, '(?m)^Completed (\d+) frames in ([\d.eE+-]+) s; average ([\d.eE+-]+) FPS; GL errors: (\d+)')
        $hardware = [regex]::Match($log, '(?m)^Renderer: ([^\r\n]+)')
        $driver = [regex]::Match($log, '(?m)^OpenGL: ([^\r\n]+)')
        if ($exitCode -ne 0 -or !$summary.Success -or $summary.Groups[1].Value -ne '1800' -or !$hardware.Success -or !$driver.Success) {
            throw "Incomplete measurement (exit $exitCode). See $logPath"
        }
        [pscustomobject]@{
            Quality = $quality
            Bloom = $bloom
            Width = 1280
            Height = 720
            Window = $windowMode
            Scene = 'Explore overview, default UI'
            Frames = 1800
            Seconds = $summary.Groups[2].Value
            AverageFPS = $summary.Groups[3].Value
            GLErrors = [int]$summary.Groups[4].Value
            Renderer = $hardware.Groups[1].Value.Trim()
            OpenGL = $driver.Groups[1].Value.Trim()
            Arguments = $startInfo.Arguments
        }
    }
}
$csv = Join-Path $runDirectory.FullName 'benchmark.csv'
$rows | Export-Csv -LiteralPath $csv -NoTypeInformation -Encoding UTF8
Write-Host "Measurements and console logs saved in $($runDirectory.FullName)"
if ($rows | Where-Object { $_.GLErrors -ne 0 }) { throw "OpenGL errors recorded in $csv" }
