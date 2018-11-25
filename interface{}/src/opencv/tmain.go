package main

//import (
//	"fmt"
//)

//func init() {

//	return
//}

//func main() {

//	fmt.Println("Entry main")
//	return
//}

import (
	"fmt"
	"opencv-master"
	"os"
)

func main() {

	fmt.Println("Entry main")
	filename := "./samples/lena.jpg"
	if len(os.Args) == 2 {
		filename = os.Args[1]
	}

	image := opencv.LoadImage(filename)
	if image == nil {
		panic("LoadImage fail")
	}
	defer image.Release()

	win := opencv.NewWindow("Go-OpenCV")
	defer win.Destroy()

	win.SetMouseCallback(func(event, x, y, flags int) {
		fmt.Printf("event = %d, x = %d, y = %d, flags = %d\n",
			event, x, y, flags,
		)
	})
	win.CreateTrackbar("Thresh", 1, 100, func(pos int) {
		fmt.Printf("pos = %d\n", pos)
	})

	win.ShowImage(image)

	opencv.WaitKey(0)
}
