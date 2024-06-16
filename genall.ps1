$ver = "2.206"
$platforms = @("win", "imac", "m1", "ios")

foreach ($platform in $platforms) {
    $command = "./bin/broma2ida.exe bindings/$ver/GeometryDash.bro $platform > out/${ver}_$platform.idc"
    Invoke-Expression $command
}