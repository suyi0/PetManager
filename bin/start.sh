# 项目启动脚本

# 先设置正确的 PATH
export PATH="/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin:$PATH"
export PATH="$HOME/.npm-global/bin:$PATH"
export PATH="$PATH:/usr/local/mysql/bin"
export PATH="/opt/homebrew/opt/curl/bin:$PATH"
export PATH="$PATH:/Applications/Tailscale.app/Contents/MacOS"
export PATH="/opt/homebrew/opt/mysql-client/bin:$PATH"

# 获取项目根目录
PROJECT_ROOT="/Users/yanghang/Code/PetManager"
cd "$PROJECT_ROOT"

# 加载环境变量
if [ -f ".env" ]; then
    # 使用 source 命令加载环境变量，更可靠
    set -a
    source .env
    set +a
    echo "✅ Environment variables loaded"
else
    echo "⚠️  Warning: .env file not found"
fi


# 检查Python依赖
echo "Checking Python dependencies..."
python3 -c "import alibabacloud_dypnsapi20170525" 2>/dev/null
if [ $? -ne 0 ]; then
    echo "Installing Python dependencies..."
    pip3 install alibabacloud-dypnsapi20170525 alibabacloud-credentials alibabacloud-tea-openapi alibabacloud-tea-util
fi

# 编译项目
echo "Building project..."
./bin/build.sh

if [ $? -eq 0 ]; then
    echo "✅ Build successful"
    
    # 启动服务
    echo "🚀 Starting PetManager service..."
    cd pethospital/backend
    ./main
else
    echo "❌ Build failed"
    exit 1
fi