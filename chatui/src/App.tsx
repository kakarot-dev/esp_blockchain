import { useState } from "react";
import "./App.css";

function App() {
    const [messages, setMessages] = useState<string[]>([]);
    const [message, setMessage] = useState("");

    const sendMessage = () => {
        if (message.trim()) {
            setMessages([...messages, message]);
            setMessage("");
        }
    };

    return (
        <div className="container">
            <h1>ESP32 Chat</h1>
            <div className="chat-box">
                {messages.map((msg, index) => (
                    <p key={index}>{msg}</p>
                ))}
            </div>
            <input
                type="text"
                value={message}
                onChange={(e) => setMessage(e.target.value)}
                placeholder="Type a message..."
            />
            <button onClick={sendMessage}>Send</button>
        </div>
    );
}

export default App;
