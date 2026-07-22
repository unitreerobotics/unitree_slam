#!/bin/bash

# 检查是否提供了路径参数
if [ $# -ne 1 ]; then
    echo "Usage: $0 <directory_path>"
    echo "Example: $0 /path/to/directory"
    exit 1
fi

# 检查提供的路径是否存在
if [ ! -d "$1" ]; then
    echo "Error: Directory '$1' does not exist"
    exit 1
fi

# 显示将要执行的操作
echo "Will delete files older than 7 days in directory: $1"

# 使用find命令查找并删除超过7天的文件
# -type f: 只查找文件
# -mtime +7: 查找修改时间超过7天的文件
# -exec rm -f {} \;: 对每个找到的文件执行rm命令
find "$1" -type f -mtime +7 -exec rm -f {} \;

echo "Cleanup completed!" 
