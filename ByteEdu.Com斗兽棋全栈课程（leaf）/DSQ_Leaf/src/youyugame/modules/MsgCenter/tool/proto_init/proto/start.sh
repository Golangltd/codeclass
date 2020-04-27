echo "删除老的protobuf文件"
rm -Rf ../protobuf/*

echo "创建新的老的protobuf文件"
./protoc Poker_Proto.proto --go_out=../protobuf --plugin=./protoc-gen-go
for file in *.proto; do
  if [ "$file" == "Poker_Proto.proto" ]; then
    continue
  fi
  echo "生成文件:"$file
  dirname=${file%%.*}
  mkdir ../protobuf/$dirname
  ./protoc $file --go_out=../protobuf/$dirname --plugin=./protoc-gen-go

done

echo "生成完毕"
