package network

type Processor interface {
	// must goroutine safe
	Route(msg interface{}, userData interface{}) error
	// must goroutine safe
	Unmarshal(data []byte) (interface{}, error)
	// must goroutine safe
	Decode(data []byte) (interface{}, error)
	Marshal(msg interface{}) ([][]byte, error)
}
