@echo off

protoc Poker_Proto.proto --go_out=../protobuf

echo 开始生成proto文件
for %%s in (*.proto) do (
    if not %%s == Poker_Proto.proto (
    echo 生成 %%~ns 文件
    cd ../protobuf
    mkdir %%~ns
    cd ../proto
    protoc %%s --go_out=../protobuf/%%~ns
    )
)

echo 删除proto临时文件夹
rd /q /s pokerking

echo 生成完成！
pause 