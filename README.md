# MPool

MySQL 数据库连接池的实现与 SGI STL 二级空间配置器、Nginx 内存池的源码剖析。

## 源码目录

| 项目 | 路径 |
| --- | --- |
| Nginx 内存池 | `My_Memory_Poll/my_Nginx/my_Nginx/` |
| SGI STL 二级空间配置器 | `My_Memory_Poll/my_SGI_STL/my_SGI_STL/` |
| MySQL 数据库连接池 | `MYSQL_connection_pool/` |

C++ 源码与测试示例位于上表目录。MySQL 项目的 5 个 Visual Studio 工程和资源文件集中存放在 `MYSQL_connection_pool/VisualStudio/`；不包含 IDE 缓存、编译产物和第三方 DLL。

## 构建与运行

- 使用支持 `.slnx` 的 Visual Studio 打开各项目的解决方案文件。原工程使用 C++20、MSVC v145 和 Windows SDK 10.0；如本机版本不同，请按需重定向工具集。
- MySQL 项目的打开入口为 [`MYSQL_connection_pool/VisualStudio/MYSQL_connection_pool.slnx`](MYSQL_connection_pool/VisualStudio/MYSQL_connection_pool.slnx)。从 VS 启动调试时，工作目录设置为上一级源码目录，以读取该目录中的 `mysql.ini`。
- MySQL 连接池依赖 MySQL C API。原工程在 `Debug | x64` 配置中使用 `C:\Program Files\MySQL\MySQL Server 8.0\include` 和 `lib`，并链接 `libmysql.lib`；请根据本机安装位置及所选配置调整包含目录、库目录和链接依赖，并将所需运行时 DLL 放入可执行文件目录或 `PATH`。
- 将 `MYSQL_connection_pool/mysql.ini.example` 复制为同目录下的 `mysql.ini`，填写本机数据库配置。运行时将该文件放在程序工作目录中。
- 上传版本中，配置模板及 `main.cpp` 直接连接示例的密码统一替换为 `YOUR_MYSQL_PASSWORD`，运行前需在本地填写。`mysql.ini` 已加入忽略规则。
- 连接池性能测试会向 `user` 表插入数据并清空该表，请使用专门的测试数据库，并按示例 SQL 准备表结构。
