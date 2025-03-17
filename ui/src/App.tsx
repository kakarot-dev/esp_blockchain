import React, { useState, useEffect, useRef } from 'react';
import './App.css';

interface Message {
  sender: string;
  message: string;
  timestamp: string;
  group: string;
}

const App: React.FC = () => {
  const [messages, setMessages] = useState<Message[]>([]);
  const [inputMessage, setInputMessage] = useState('');
  const [username, setUsername] = useState('');
  const [currentGroup, setCurrentGroup] = useState('General');
  const [groups] = useState(['General', 'Secret', 'Talks', 'Random']);
  const [sidebarOpen, setSidebarOpen] = useState(false);
  const messagesEndRef = useRef<HTMLDivElement>(null);
  const inputRef = useRef<HTMLInputElement>(null);

  useEffect(() => {
    const storedUsername = localStorage.getItem('chatUsername') || 
      `User${Math.floor(Math.random() * 10000)}`;
    setUsername(storedUsername);
    localStorage.setItem('chatUsername', storedUsername);
  }, []);

  // Fetch messages every 1 second
  useEffect(() => {
    const fetchMessages = async () => {
      try {
        const response = await fetch(`/api/messages?group=${currentGroup}`);
        if (response.ok) {
          setMessages(await response.json());
        }
      } catch (error) {
        console.error('Error fetching messages:', error);
      }
    };

    fetchMessages(); // Initial fetch
    const interval = setInterval(fetchMessages, 1000);
    return () => clearInterval(interval);
  }, [currentGroup]);

  useEffect(() => {
    messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
  }, [messages]);

  const toggleSidebar = () => setSidebarOpen(!sidebarOpen);

  const selectGroup = (group: string) => {
    setCurrentGroup(group);
    setSidebarOpen(false);
    setTimeout(() => inputRef.current?.focus(), 300);
  };

  const sendMessage = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!inputMessage.trim()) return;

    try {
      const formData = new URLSearchParams();
      formData.append('sender', username);
      formData.append('message', inputMessage);
      formData.append('group', currentGroup);

      await fetch('/api/messages', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: formData,
      });

    } catch (error) {
      console.error('Error sending message:', error);
    }

    setInputMessage('');
    inputRef.current?.focus();
  };

  const formatTimestamp = (timestamp: string) => {
    try {
      return new Date(timestamp).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
    } catch {
      return '??:??';
    }
  };

  return (
    <div className="app-container">
      <div className={`sidebar ${sidebarOpen ? 'open' : ''}`}>
        <div className="groups-header">
          <h2>Groups</h2>
          <button className="close-sidebar" onClick={toggleSidebar}>×</button>
        </div>
        <div className="group-list">
          {groups.map((group) => (
            <div
              key={group}
              className={`group-item ${currentGroup === group ? 'active-group' : ''}`}
              onClick={() => selectGroup(group)}
            >
              {group}
            </div>
          ))}
        </div>
      </div>

      {sidebarOpen && <div className="overlay" onClick={toggleSidebar}></div>}

      <div className="main-content">
        <div className="header">
          <button className="hamburger" onClick={toggleSidebar}>
            <span></span><span></span><span></span>
          </button>
          <h1 className="title">EmbedCord</h1>
          <div className="user-info">{username}</div>
        </div>

        <div className="chat-area">
          <div className="group-header">
            <h2>#{currentGroup}</h2>
          </div>

          <div className="messages-container">
            {messages.length === 0 ? (
              <div className="empty-messages">No messages in #{currentGroup}</div>
            ) : (
              messages.map((msg, index) => (
                <div key={`${msg.timestamp}-${index}`} className={`message ${msg.sender === username ? 'own-message' : ''}`}>
                  <div className="message-header">
                    <span className="message-sender">{msg.sender}</span>
                    <span className="message-time">{formatTimestamp(msg.timestamp)}</span>
                  </div>
                  <div className="message-content">{msg.message}</div>
                </div>
              ))
            )}
            <div ref={messagesEndRef} />
          </div>

          <form className="input-form" onSubmit={sendMessage}>
            <input
              ref={inputRef}
              type="text"
              className="message-input"
              value={inputMessage}
              onChange={(e) => setInputMessage(e.target.value)}
              placeholder={`Message #${currentGroup}`}
              autoComplete="off"
            />
            <button type="submit" className="send-button" disabled={inputMessage.length > 300}>→</button>
          </form>
        </div>
        {inputMessage.length > 300 && <p className="char-warning">Message too long! Max 300 characters.</p>}
      </div>
    </div>
  );
};

export default App;
