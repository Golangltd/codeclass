package main

import "fmt"

func main() {

	// slice 的遍历
	nums := []int{2, 3, 4}
	sum := 0

	for _, num := range nums {
		sum += num
	}

	fmt.Println("sum:", sum)

	//获取slice的索引值
	for i, num := range nums {
		if num == 3 {
			fmt.Println("index:", i)
		}
	}

	// map
	kvs := map[string]string{"a": "apple", "b": "banana"}
	for k, v := range kvs {
		fmt.Println("%s-> %s", k, v)
	}

}
