//   Copyright 2009-2012 Joubin Houshyar
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.

// Blackbox tests for protocol.go's exported functions

package test

import (
	"bufio"
	"bytes"
	"fmt"
	"redis"
	"testing"
)

func TestGetPubSubResponse(t *testing.T) {

	// Get test data
	reader, expectedMessages := getTestReaderAndMessages()

	// Test
	for i, expectedMessage := range expectedMessages {
		msg, e := redis.GetPubSubResponse(reader)
		if e != nil {
			t.Fatalf("TestGetPubSubResponse - %s", e)
		} else if msg == nil {
			t.Fatalf("TestGetPubSubResponse - nil message given nil error")
		}
		verifyMessage(fmt.Sprintf("TestGetPubSubResponse - msg %d", i), t, expectedMessage, msg)
	}
}

// verifies obtained message against expected
// Note: func uses t.Errorf and NOT Fatalf.

func verifyMessage(info string, t *testing.T, expected, got *redis.Message) {
	//	fmt.Printf("compare:\n\t%s\n\t%s\n", expected, got)
	var expectedMType, gotMType redis.PubSubMType
	var expectedTopic, gotTopic string
	var expectedBody, gotBody []byte
	var expectedSubCnt, gotSubCnt int

	expectedMType = expected.Type
	gotMType = got.Type
	if gotMType != expectedMType {
		t.Errorf("%s - Type check - expected:%s got:%s", info, expectedMType, gotMType)
	}
	expectedTopic = expected.Topic
	gotTopic = got.Topic
	if gotTopic != expectedTopic {
		t.Errorf("%s - Topic check - expected:%s got:%s", info, expectedTopic, gotTopic)
	}
	expectedBody = expected.Body
	gotBody = got.Body
	if !compareByteArrays(gotBody, expectedBody) {
		t.Errorf("%s - Body check - expected:%s got:%s", info, expectedBody, gotBody)
	}
	expectedSubCnt = expected.SubscriptionCnt
	gotSubCnt = got.SubscriptionCnt
	if gotSubCnt != expectedSubCnt {
		t.Errorf("%s - SubscriptionCnt check - expected:%s got:%s", info, expectedSubCnt, gotSubCnt)
	}
}

func getTestReaderAndMessages() (*bufio.Reader, []*redis.Message) {
	var buf bytes.Buffer
	var expectedMessage *redis.Message
	expected := make([]*redis.Message, 0)

	// SUBSCRIBE to topic-1
	// scnt 1
	buf.WriteString("*3\r\n")
	buf.WriteString("$9\r\n")
	buf.WriteString("subscribe\r\n")
	buf.WriteString("$7\r\n")
	buf.WriteString("topic-1\r\n")
	buf.WriteString(":1\r\n")

	expectedMessage = &redis.Message{
		Type:            redis.SUBSCRIBE_ACK,
		Topic:           "topic-1",
		SubscriptionCnt: 1,
	}
	expected = append(expected, expectedMessage)

	// SUBSCRIBE to topic-2
	// scnt 2
	buf.WriteString("*3\r\n")
	buf.WriteString("$9\r\n")
	buf.WriteString("subscribe\r\n")
	buf.WriteString("$7\r\n")
	buf.WriteString("topic-2\r\n")
	buf.WriteString(":2\r\n")

	expectedMessage = &redis.Message{
		Type:            redis.SUBSCRIBE_ACK,
		Topic:           "topic-2",
		SubscriptionCnt: 2,
	}
	expected = append(expected, expectedMessage)

	// PSUBSCRIBE to topics/dujour/*
	// scnt 3
	buf.WriteString("*3\r\n")
	buf.WriteString("$10\r\n")
	buf.WriteString("psubscribe\r\n")
	buf.WriteString("$15\r\n")
	buf.WriteString("topics/dujour/*\r\n")
	buf.WriteString(":3\r\n")

	expectedMessage = &redis.Message{
		Type:            redis.SUBSCRIBE_ACK,
		Topic:           "topics/dujour/*",
		SubscriptionCnt: 3,
	}
	expected = append(expected, expectedMessage)

	// message to topic-1
	buf.WriteString("*3\r\n")
	buf.WriteString("$7\r\n")
	buf.WriteString("message\r\n")
	buf.WriteString("$7\r\n")
	buf.WriteString("topic-1\r\n")
	buf.WriteString("$7\r\n")
	buf.WriteString("Salaam!\r\n")

	expectedMessage = &redis.Message{
		Type:  redis.MESSAGE,
		Topic: "topic-1",
		Body:  []byte("Salaam!"),
	}
	expected = append(expected, expectedMessage)

	// message to topic-2
	buf.WriteString("*3\r\n")
	buf.WriteString("$7\r\n")
	buf.WriteString("message\r\n")
	buf.WriteString("$7\r\n")
	buf.WriteString("topic-2\r\n")
	buf.WriteString("$7\r\n")
	buf.WriteString("Salaam!\r\n")

	expectedMessage = &redis.Message{
		Type:  redis.MESSAGE,
		Topic: "topic-2",
		Body:  []byte("Salaam!"),
	}
	expected = append(expected, expectedMessage)

	// UNSUBSCRIBE from topic-1
	// scnt 1
	buf.WriteString("*3\r\n")
	buf.WriteString("$11\r\n")
	buf.WriteString("unsubscribe\r\n")
	buf.WriteString("$7\r\n")
	buf.WriteString("topic-1\r\n")
	buf.WriteString(":1\r\n")

	expectedMessage = &redis.Message{
		Type:            redis.UNSUBSCRIBE_ACK,
		Topic:           "topic-1",
		SubscriptionCnt: 1,
	}
	expected = append(expected, expectedMessage)

	return bufio.NewReader(&buf), expected
}
