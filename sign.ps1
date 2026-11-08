# sign.ps1 — runs elevated (as Administrator) to sign keytech.exe
# Called automatically by build.bat
param([string]$ExePath)

if (-not $ExePath) {
    Write-Host "Usage: sign.ps1 <path-to-exe>"
    exit 1
}

# Look for an existing keytech code-signing cert in LocalMachine\My
$cert = Get-ChildItem Cert:\LocalMachine\My -CodeSigningCert |
        Where-Object { $_.Subject -like '*keytech*' } |
        Select-Object -First 1

if (-not $cert) {
    Write-Host "Creating self-signed code-signing certificate..."
    $cert = New-SelfSignedCertificate `
        -Subject "CN=keytech" `
        -Type CodeSigning `
        -CertStoreLocation Cert:\LocalMachine\My `
        -NotAfter (Get-Date).AddYears(10)

    # Trust the cert as a root CA so Windows validates it
    $rootStore = New-Object System.Security.Cryptography.X509Certificates.X509Store("Root", "LocalMachine")
    $rootStore.Open("ReadWrite")
    $rootStore.Add($cert)
    $rootStore.Close()

    # Trust the publisher so AppLocker lets it run
    $pubStore = New-Object System.Security.Cryptography.X509Certificates.X509Store("TrustedPublisher", "LocalMachine")
    $pubStore.Open("ReadWrite")
    $pubStore.Add($cert)
    $pubStore.Close()

    Write-Host "Certificate created and added to Trusted Root + Trusted Publishers."
} else {
    Write-Host "Re-using existing keytech certificate."
}

$result = Set-AuthenticodeSignature -FilePath $ExePath -Certificate $cert
if ($result.Status -eq "Valid") {
    Write-Host "Signed OK: $ExePath"
} else {
    Write-Host "Sign result: $($result.Status) — $($result.StatusMessage)"
}

Read-Host "Press Enter to close"
