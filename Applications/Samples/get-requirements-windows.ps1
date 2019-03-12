
if ($true) {

    Write-Error "This script is obsolete. Please work under WSL and run build-wasm.sh"

} else {

    param(
        [IO.DirectoryInfo] $EmsdkRootDir = "C:\Emscripten",
        [bool] $Overwrite = $false
      )
    
    if (Test-Path -Path $EmsdkRootDir) {
        if( $Override) {
            Remove-Item -Path $EmsdkRootDir -Force -Recurse
        } else {
            throw "The `"$EmsdkRootDir`" folder may not exist! Use the Overwrite flag to bypass this check."
        }
    }
    
    # TODO: detect whether git is installed
    # choco install -y git
    
    Write-Host "Will retrieve the Emscripten SDK to the `"$EmsdkRootDir`" folder"
    
    $EmsdkParentDir = split-path -Parent $EmsdkRootDir
    $EmsdkRootName = split-path -Leaf $EmsdkRootDir
    
    Push-Location $EmsdkParentDir
    
    git clone https://github.com/juj/emsdk.git $EmsdkRootName
    cd $EmsdkRootName
    
    git pull
    
    ./emsdk install latest
    
    ./emsdk activate latest
    
    echo "INFO: the ~/.emscripten file has been configured for this installation of Emscripten."
    
    Write-Host "emsdk is now installed in $EmsdkRootDir"
    
    Pop-Location

}




