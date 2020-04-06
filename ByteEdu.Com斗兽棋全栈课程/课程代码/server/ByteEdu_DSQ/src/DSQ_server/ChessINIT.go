package main

import (
	"DSQ_server/Proto"
)

/*

------------------------------------
|   [0,0]       [1,0]      [2,0]      [3,0]   |
|                                             |
|   [0,1]       [1,1]      [2,1]      [3,1]   |
|                                             |
|   [0,2]       [1,2]      [2,2]      [3,2]   |
|                                             |
|   [0,3]       [1,3]      [2,3]      [3,3]   |
|-----------------------------------
                服务器棋牌设计
*/

// 棋牌的初始化数据
// 1-16
var DSQ_QI = []int{
	Proto_DSQGame.Elephant,
	Proto_DSQGame.Lion,
	Proto_DSQGame.Tiger,
	Proto_DSQGame.Leopard,
	Proto_DSQGame.Wolf,
	Proto_DSQGame.Dog,
	Proto_DSQGame.Cat,
	Proto_DSQGame.Mouse,
	Proto_DSQGame.Elephant + Proto_DSQGame.Mouse,
	Proto_DSQGame.Lion + Proto_DSQGame.Mouse,
	Proto_DSQGame.Tiger + Proto_DSQGame.Mouse,
	Proto_DSQGame.Leopard + Proto_DSQGame.Mouse,
	Proto_DSQGame.Wolf + Proto_DSQGame.Mouse,
	Proto_DSQGame.Dog + Proto_DSQGame.Mouse,
	Proto_DSQGame.Cat + Proto_DSQGame.Mouse,
	Proto_DSQGame.Mouse + Proto_DSQGame.Mouse,
}
