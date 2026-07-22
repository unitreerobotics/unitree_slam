WorkDir=$(cd $(dirname $0); pwd)
echo "WrokDir=$WorkDir"

echo "Delete the original directory and install......"
rm -r /unitree/module/unitree_slam
set -e
cp -r $WorkDir/unitree_slam  /unitree/module
chmod 777 -R /unitree/module/unitree_slam

echo "Set NX power mode to 0......" 
nvpmodel -m 0

echo "Modify the size of the data buffer......" 
SYSCTL_FILE="/etc/sysctl.conf"
set_sysctl() {
    local key="$1"
    local value="$2"

    if grep -q "^${key}=" "$SYSCTL_FILE"; then
        sed -i "s|^${key}=.*|${key}=${value}|" "$SYSCTL_FILE"
    else
        echo "${key}=${value}" >> "$SYSCTL_FILE"
    fi
}
set_sysctl net.core.rmem_max 52428800
set_sysctl net.core.rmem_default 52428800
set_sysctl net.core.wmem_max 52428800
set_sysctl net.core.wmem_default 52428800
set_sysctl net.core.netdev_max_backlog 5000
sysctl -p "$SYSCTL_FILE"

echo "Insatll success!"
