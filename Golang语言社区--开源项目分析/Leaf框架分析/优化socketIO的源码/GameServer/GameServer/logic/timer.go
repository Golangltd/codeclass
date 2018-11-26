package logic

import "time"

type TimerThinker struct {
	Callback func([]interface{})
	Args     []interface{}
	Close    chan bool
}

//每秒执行一次
func (t *TimerThinker) StartTimer() {
	ticker := time.NewTicker(time.Second * 1)
	go func() {
		for {
			select {
			case <-ticker.C:
				t.Callback(t.Args)
			case <-t.Close:
				return
			}
		}

	}()
}

//N秒后执行一次
func (t *TimerThinker) StartTimerByX(tt time.Duration) {
	ticker := time.NewTicker(time.Second * 1)
	var c time.Duration
	go func() {
		for {
			select {
			case <-ticker.C:
				c = c + time.Millisecond*100
				if c >= tt*time.Millisecond {
					t.Callback(t.Args)
					return
				}
			case <-t.Close:
				return
			}
		}

	}()
}

//N秒执行一次
func (t *TimerThinker) StartTimerByXM(tt time.Duration) {
	ticker := time.NewTicker(time.Second * 1)
	var c time.Duration
	go func() {
		for {
			select {
			case <-ticker.C:
				c = c + time.Millisecond*100
				if c >= tt*time.Millisecond {
					t.Callback(t.Args)
				}
			case <-t.Close:
				return
			}
		}

	}()
}

//1小时秒后执行一次
func (t *TimerThinker) StartTimerByHour() {
	T := time.NewTimer(time.Second * 3600)
	go func() {
		for {
			select {
			case <-T.C:
				t.Callback(t.Args)
				return
			case <-t.Close:
				return
			}
		}
	}()

}

//关闭计时器

func (t *TimerThinker) CloseTimer() {
	t.Close <- true
}
