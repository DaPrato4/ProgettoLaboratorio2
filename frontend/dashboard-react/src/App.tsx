import { useEffect, useRef, useState } from "react";

const CELL_SIZE = 2; // in pixel
const GRID_WIDTH = 400;
const GRID_HEIGHT = 300;

type Pos = { x: number; y: number };
type Rescuer = { 
  id: number; 
  type: string;
  pos: Pos; 
  status: string
  color?: string
};
type Emergency = { 
  id: number; 
  type: string;
  pos: Pos; 
  status: string;
  color?: string
};
type Animation = {
  id: number;
  from: Pos;
  to: Pos;
  duration: number; // in ms
  startTime: number; // timestamp
};

function App() {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const [animations, setAnimations] = useState<Animation[]>([]);
  const colorMapRef = useRef<Record<string, string>>({});
  const wsRef = useRef<WebSocket | null>(null);

  //POSIZIONI DI PROVA TEMPORANEE
  const [emergencies,setEmergencies] = useState<Emergency[]>([]);
  const [rescuers, setRescuers] = useState<Rescuer[]>([]);



  // Funzione per avviare l'animazione
  function moveRescuerTo(id: number, to: Pos, seconds: number) {
    setRescuers((prevRescuers) => {
      const rescuer = prevRescuers.find((r) => r.id === id);
      if (!rescuer) return prevRescuers;

      // Setta l'animazione con i dati CORRETTI
      setAnimations((prevAnims) => [
        ...prevAnims.filter((a) => a.id !== id),
        {
          id,
          from: { ...rescuer.pos },
          to,
          duration: seconds * 1000,
          startTime: performance.now(),
        },
      ]);

      return prevRescuers.map((r) =>
        r.id === id ? { ...r, target: to } : r
      );
    });
  }

// "#ef4444"
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    function draw() {
      if (!ctx || !canvas) return;
      ctx.clearRect(0, 0, canvas.width, canvas.height);

      ctx.strokeStyle = "#2d2d2d";
      ctx.lineWidth = 2;
      ctx.strokeRect(0, 0, GRID_WIDTH * CELL_SIZE, GRID_HEIGHT * CELL_SIZE);

      emergencies.forEach((em) => {
        ctx.fillStyle = em.color || "#ef4444";
        const offsetX = -2.5 * CELL_SIZE;
        const offsetY = -2.5 * CELL_SIZE;
        ctx.beginPath();
        ctx.moveTo(em.pos.x * CELL_SIZE - CELL_SIZE*2 + 4 + offsetX, em.pos.y * CELL_SIZE - CELL_SIZE*2 + offsetY);
        ctx.lineTo(em.pos.x * CELL_SIZE - CELL_SIZE*2 + CELL_SIZE * 10 - 4 + offsetX, em.pos.y * CELL_SIZE - CELL_SIZE*2 + offsetY);
        ctx.quadraticCurveTo(
          em.pos.x * CELL_SIZE - CELL_SIZE*2 + CELL_SIZE * 10 + offsetX,
          em.pos.y * CELL_SIZE - CELL_SIZE*2 + offsetY,
          em.pos.x * CELL_SIZE - CELL_SIZE*2 + CELL_SIZE * 10 + offsetX,
          em.pos.y * CELL_SIZE - CELL_SIZE*2 + 4 + offsetY
        );
        ctx.lineTo(em.pos.x * CELL_SIZE - CELL_SIZE*2 + CELL_SIZE * 10 + offsetX, em.pos.y * CELL_SIZE - CELL_SIZE*2 + CELL_SIZE * 10 - 4 + offsetY);
        ctx.quadraticCurveTo(
          em.pos.x * CELL_SIZE - CELL_SIZE*2 + CELL_SIZE * 10 + offsetX,
          em.pos.y * CELL_SIZE - CELL_SIZE*2 + CELL_SIZE * 10 + offsetY,
          em.pos.x * CELL_SIZE - CELL_SIZE*2 + CELL_SIZE * 10 - 4 + offsetX,
          em.pos.y * CELL_SIZE - CELL_SIZE*2 + CELL_SIZE * 10 + offsetY
        );
        ctx.lineTo(em.pos.x * CELL_SIZE - CELL_SIZE*2 + 4 + offsetX, em.pos.y * CELL_SIZE - CELL_SIZE*2 + CELL_SIZE * 10 + offsetY);
        ctx.quadraticCurveTo(
          em.pos.x * CELL_SIZE - CELL_SIZE*2 + offsetX,
          em.pos.y * CELL_SIZE - CELL_SIZE*2 + CELL_SIZE * 10 + offsetY,
          em.pos.x * CELL_SIZE - CELL_SIZE*2 + offsetX,
          em.pos.y * CELL_SIZE - CELL_SIZE*2 + CELL_SIZE * 10 - 4 + offsetY
        );
        ctx.lineTo(em.pos.x * CELL_SIZE - CELL_SIZE*2 + offsetX, em.pos.y * CELL_SIZE - CELL_SIZE*2 + 4 + offsetY);
        ctx.quadraticCurveTo(
          em.pos.x * CELL_SIZE - CELL_SIZE*2 + offsetX,
          em.pos.y * CELL_SIZE - CELL_SIZE*2 + offsetY,
          em.pos.x * CELL_SIZE - CELL_SIZE*2 + 4 + offsetX,
          em.pos.y * CELL_SIZE - CELL_SIZE*2 + offsetY
        );
        ctx.closePath();
        ctx.fill();
      });

      rescuers.forEach(({ pos, color }) => {
        const cx = pos.x * CELL_SIZE + CELL_SIZE / 2;
        const cy = pos.y * CELL_SIZE + CELL_SIZE / 2;
        ctx.fillStyle = color || "#3b82f6";
        ctx.beginPath();
        ctx.arc(cx, cy, CELL_SIZE * 3, 0, Math.PI * 2);
        ctx.fill();
      });
    }

    let animationFrame: number;

    function animate(now: number) {
      setRescuers((prev) =>
        prev.map((r) => {
          const anim = animations.find((a) => a.id === r.id);
          if (!anim) return r;

          const elapsed = now - anim.startTime;
          if (elapsed >= anim.duration) {
            // Fine animazione
            setAnimations((prevAnims) => prevAnims.filter((a) => a.id !== r.id));
            return { ...r, pos: { ...anim.to } };
          }
          // Interpolazione lineare
          const t = elapsed / anim.duration;
          return {
            ...r,
            pos: {
              x: anim.from.x + (anim.to.x - anim.from.x) * t,
              y: anim.from.y + (anim.to.y - anim.from.y) * t,
            },
          };
        })
      );
      draw();
      animationFrame = requestAnimationFrame(animate);
    }

    draw();
    animationFrame = requestAnimationFrame(animate);

    return () => cancelAnimationFrame(animationFrame);
  }, [emergencies, rescuers, animations]);
  
  useEffect(() => {
  const ws = new WebSocket("ws://172.28.236.125:8080");
  wsRef.current = ws;

  ws.onopen = () => console.log("✅ WebSocket connessa a Node.js");
  ws.onclose = () => console.warn("❌ WebSocket chiusa");

  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);
      const { id, event: evt } = data;

      if (evt === "RESCUER_INIT") {
        const { x, y, type } = data;
        const newId = parseInt(id);

        setRescuers((prev) => {
          if (prev.some((r) => r.id === newId)) return prev;

          // Usa la mappa globale per assegnare il colore per tipo
          let color = colorMapRef.current[type];
          if (!color) {
            color = "#" + Math.floor(Math.random() * 16777215).toString(16).padStart(6, "0");
            colorMapRef.current[type] = color;
          }

          return [
            ...prev,
            {
              id: newId,
              type,
              pos: { x, y },
              status: "IDLE",
              color,
            },
          ];
        });
      }else if (evt === "EMERGENCY_INIT") {
        const { x, y, type,  status } = data;
        const newId = parseInt(id);

        setEmergencies((prev) => {
          if (prev.some((e) => e.id === newId)) return prev;

          return [
            ...prev,
            {
              id: newId,
              type,
              pos: { x, y },
              status,
              color: "#ef4444",
            },
          ];
        });
      }else if (evt === "RESCUER_STATUS") {
        const { x, y,  status, time } = data;
        const newId = parseInt(id);

        if (status != "IDLE" && status != "ON_SCENE" && time > 0) {
          moveRescuerTo(newId, { x: x, y: y }, time);
        }

        setRescuers((prev) =>
          prev.map((r) =>
            r.id === newId ? { ...r, status } : r
          )
        );

      }else if (evt === "EMERGENCY_STATUS") {
        const {status} = data;
        const newId = parseInt(id);

        setEmergencies((prev) =>
          prev.map((r) =>
            r.id === newId ? { ...r, status } : r
          )
        );

        if (status === "TIMEOUT" || status === "COMPLETED") {
          let t;
          (status === "TIMEOUT") ? t = "#000" : t = "#00ff00";
          setEmergencies((prev) => prev.map((e) => (e.id === newId ? { ...e, color:t} : e))); 
          setTimeout(() => {
            setEmergencies((prev) => prev.filter((e) => e.id !== newId)); 
          }, 5000);
          return;
        }

      }

    } catch (err) {
      console.error("❌ Errore parsing messaggio WebSocket:", event.data);
    }
  };

  return () => ws.close();
}, []);

  return (
    <div className="bg-gray-900 min-h-screen flex flex-col items-center justify-center text-white">
      <h1 className="text-4xl font-bold mb-6 mt-6">🚨 Emergency Grid Monitor</h1>

      
      <form
        className="flex flex-row gap-4 mb-8 items-end bg-blue-600 p-4 rounded-2xl border-red-600 border-2"
        onSubmit={e => {
          e.preventDefault();
          const form = e.target as HTMLFormElement;
          const name = (form.elements.namedItem('name') as HTMLInputElement).value;
          const x = (form.elements.namedItem('x') as HTMLInputElement).value;
          const y = (form.elements.namedItem('y') as HTMLInputElement).value;
          const delay = (form.elements.namedItem('delay') as HTMLInputElement).value;

          if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
            wsRef.current.send(JSON.stringify({
              event: "EMERGENCY_CREATE",
              name,
              x,
              y,
              delay,
            }));
          } else {
            alert("WebSocket non connessa!");
          }
          form.reset();
        }}
      >
        <div className="flex flex-col">
          <label htmlFor="name" className="mb-1">Tipo emergenza</label>
          <input
            id="name"
            name="name"
            type="text"
            required
            className="rounded px-2 py-1 text-black bg-blue-300 border-1 border-blue-800"
            placeholder="Blackout"
          />
        </div>
        <div className="flex flex-col">
          <label htmlFor="x" className="mb-1">X</label>
          <input
            id="x"
            name="x"
            type="number"
            min={0}
            max={GRID_WIDTH}
            step={1}
            required
            className="rounded px-2 py-1 text-black bg-blue-300 border-1 border-blue-800"
            placeholder="150"
          />
        </div>
        <div className="flex flex-col">
          <label htmlFor="y" className="mb-1">Y</label>
          <input
            id="y"
            name="y"
            type="number"
            min={0}
            max={GRID_HEIGHT}
            step={1}
            required
            className="rounded px-2 py-1 text-black bg-blue-300 border-1 border-blue-800"
            placeholder="100"
          />
        </div>
        <div className="flex flex-col">
          <label htmlFor="Timestamp" className="mb-1">Timestamp (s)</label>
          <input
            id="Timestamp"
            name="Timestamp"
            type="number"
            min={0}
            step={1}
            required
            className="rounded px-2 py-1 text-black bg-blue-300 border-1 border-blue-800"
            placeholder="600"
          />
        </div>
        <button
          type="submit"
          className="bg-blue-900 hover:bg-red-600 text-white font-bold py-2 px-6 rounded"
        >
          Crea emergenza
        </button>
      </form>

      <div className="flex flex-col xl:flex-row gap-10 w-full justify-center items-center mb-20">
        <div className="flex flex-col items-center overflow-x-auto px-2">
          <canvas
            ref={canvasRef}
            width={GRID_WIDTH * CELL_SIZE}
            height={GRID_HEIGHT * CELL_SIZE}
            className="max-w-full h-auto border border-blue-500 rounded-xl bg-neutral-300"
          />
        </div>

        <div className="flex gap-3.5 justify-center items-start">
          <div className="min-w-[350px]">
            <h1 className="text-2xl font-bold mb-4">🚑 Soccorritori</h1>
            <div className=" mb-2">
              <ul className="flex pr-2 text-center">
                <li className="w-1/12">ID</li>
                <li className="w-3/12">Tipo</li>
                <li className="w-4/12">Posizione</li>
                <li className="w-4/12">Stato</li>
              </ul>
            </div>
            <ul
              className="overflow-y-scroll w-full scrollbar-overlay"
              style={{
                maxHeight: `${GRID_HEIGHT * CELL_SIZE - 90}px`,
              }}
            >
              {rescuers.map((rescuer) => (
                <li key={rescuer.id} className="mb-2 flex rounded-lg p-2 text-center" style={{ backgroundColor: rescuer.color }}>
                  <span className="w-1/12">{rescuer.id}</span>
                  <span className="w-3/12">{rescuer.type}</span>
                  <span className="w-4/12">({rescuer.pos.x.toFixed(1)}, {rescuer.pos.y.toFixed(1)})</span>
                  <span className="w-4/12">{rescuer.status}</span>
                </li>
              ))}
            </ul>
          </div>

          <div className="min-w-[350px]">
            <h1 className="text-2xl font-bold mb-4">🚨 Emergenze attive</h1>
            <div className=" mb-2">
              <ul className="flex text-center">
                <li className="w-1/12">ID</li>
                <li className="w-3/12">Tipo</li>
                <li className="w-4/12">Posizione</li>
                <li className="w-4/12">Stato</li>
              </ul>
            </div>
            <ul
              className="overflow-y-auto w-full"
              style={{
                maxHeight: `${GRID_HEIGHT * CELL_SIZE - 90}px`,
                scrollbarGutter: "stable overlay",
              }}
            >
              {emergencies.map((emergency) => (
                <li key={emergency.id} className="mb-2 flex bg-amber-600 rounded-lg p-2 text-center">
                  <span className="w-1/12">{emergency.id}</span>
                  <span className="w-3/12">{emergency.type}</span>
                  <span className="w-4/12">
                    ({emergency.pos.x.toFixed(1)}, {emergency.pos.y.toFixed(1)})
                  </span>
                  <span className="w-4/12 p-1 rounded-lg border-1 border-blue-800" style={{ backgroundColor: emergency.color }}>{emergency.status}</span>
                </li>
              ))}
            </ul>
          </div>
        </div>
      </div>

    </div>
  );
}

export default App;
