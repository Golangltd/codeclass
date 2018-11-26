package internal

import (
	"log"
	"reflect"

	pb "GameServer/modules/CalRPC/CalCall"

	"fmt"

	"golang.org/x/net/context"
	"google.golang.org/grpc"
)

var (
	conn   *grpc.ClientConn
	caller pb.GreeterClient
)

func init() {
	Rpc()
	ChanRPC.Register("LZMJCal", LZMJCal)
}

func handler(m interface{}, h interface{}) {
	skeleton.RegisterChanRPC(reflect.TypeOf(m), h)

}

const (
	address     = "localhost:50051"
	defaultName = "world"
)

func Rpc() {
	// Set up a connection to the server.
	var err error
	conn, err = grpc.Dial(address, grpc.WithInsecure())
	if err != nil {
		log.Fatalf("did not connect: %v", err)
	}
	caller = pb.NewGreeterClient(conn)
}

//柳州麻将的计算
func LZMJCal(args []interface{}) interface{} {
	data := args[0].(*pb.MajongCal)
A:
	r, err := caller.CallCal(context.Background(), data)
	if err != nil {
		err := conn.Close()
		if err != nil {
			fmt.Println(err)
		}
		Rpc()
		goto A

	}
	return r.GetCalResult()
}
