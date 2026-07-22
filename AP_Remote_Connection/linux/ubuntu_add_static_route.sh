#!/bin/bash
set -e

# 自动检测无线网卡
WLAN_DEV=$(ip link | grep "^[0-9]: wl" | awk -F': ' '{print $2}' | cut -d' ' -f1 | head -n1)
if [ -z "$WLAN_DEV" ]; then
  echo "❌ 未找到无线网卡，请先连接 WiFi"
  exit 1
fi
echo "✅ 无线网卡：$WLAN_DEV"

# 清理旧的错误 netplan 配置
sudo rm -f /etc/netplan/02-custom-default-route.yaml
sudo rm -f /etc/netplan/02-route-wifi.yaml

# 创建 NetworkManager 自动钩子（连WiFi自动加路由，永不报错）
sudo tee /etc/NetworkManager/dispatcher.d/99-add-backup-route >/dev/null <<'EOF'
#!/bin/bash
IF="$1"
ACTION="$2"
GW="192.168.12.1"
METRIC="600"

if [[ "$ACTION" == "up" && "$IF" =~ ^wl ]]; then
  ip route del default via $GW dev $IF 2>/dev/null
  ip route add default via $GW dev $IF metric $METRIC
fi
EOF

# 权限必须正确
sudo chmod 755 /etc/NetworkManager/dispatcher.d/99-add-backup-route
sudo chown root:root /etc/NetworkManager/dispatcher.d/99-add-backup-route

# 立即添加一次（马上生效）
sudo ip route del default via 192.168.12.1 dev $WLAN_DEV 2>/dev/null
sudo ip route add default via 192.168.12.1 dev $WLAN_DEV metric 600

echo -e "\n========================================"
echo "✅ 配置完成！无任何报错！"
echo "🎯 网关：192.168.12.1"
echo "📊 路由优先级：固定 metric 600"
echo "========================================"
echo ""

ip route show default
