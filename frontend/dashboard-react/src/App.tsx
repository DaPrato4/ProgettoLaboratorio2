import { useEffect, useState } from 'react'
import reactLogo from './assets/react.svg'
import viteLogo from '/vite.svg'
import './App.css'

const ipaddress:string = '172.28.236.125'

function App() {
  // Stato per salvare i messaggi ricevuti via WebSocket
  const [messages, setMessages] = useState<string[]>([])

  useEffect(() => {
    // Apri la connessione WebSocket al server Node.js
    const ws = new WebSocket('ws://'+ipaddress+':8080')

    // Alla ricezione di un messaggio, aggiorna lo stato
    ws.onmessage = (event) => {
      setMessages(prev => [...prev, event.data])
    }

    // Pulizia: chiudi la connessione al termine del componente
    return () => {
      ws.close()
    }
  }, [])

  return (
    <>
      <div>
        <a href="https://vite.dev" target="_blank">
          <img src={viteLogo} className="logo" alt="Vite logo" />
        </a>
        <a href="https://react.dev" target="_blank">
          <img src={reactLogo} className="logo react" alt="React logo" />
        </a>
      </div>
      <h1>Emergency Dashboard</h1>
      <div className="card">
        {/* Lista messaggi ricevuti */}
        <ul>
          {messages.map((msg, index) => (
            <li key={index}>{msg}</li>
          ))}
        </ul>
      </div>
      <p className="read-the-docs">
        Click on the Vite and React logos to learn more
      </p>
    </>
  )
}

export default App
