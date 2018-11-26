package util

import (
	"errors"
)

type Stack []interface{}

func (stack *Stack) Push(v interface{}) {
	*stack = append(*stack, v)
}

func (stack *Stack) Pop() (interface{}, error) {
	if len(*stack) == 0 {
		return nil, errors.New("stack empty")
	}
	v := (*stack)[len(*stack)-1]
	*stack = (*stack)[:len(*stack)-1]
	return v, nil
}

func (stack *Stack) Top() (interface{}, error) {
	if len(*stack) == 0 {
		return nil, errors.New("stack empty")
	}

	return (*stack)[len(*stack)-1], nil
}

func (stack *Stack) Len() int {
	return len(*stack)
}
