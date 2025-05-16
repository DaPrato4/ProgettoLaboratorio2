import { useEffect, useRef, useState } from "react";

const CELL_SIZE = 2; // in pixel
const GRID_WIDTH = 400;
const GRID_HEIGHT = 300;

type Pos = { x: number; y: number };
type Rescuer = { 
  id: number; 
  type: string;
  pos: Pos; 
  target?: Pos;
  status?: string
  color?: string
};
type Emergency = { 
  id: number; 
  type: string;
  pos: Pos; 
  status: string;
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

  //POSIZIONI DI PROVA TEMPORANEE
  const [emergencies,setEmergencies] = useState<Emergency[]>([
    { id: 1, type: "incendio", pos: { x: 0, y: 0 }, status: "attivo" },
    { id: 2, type: "incendio", pos: { x: 10, y: 20 }, status: "attivo" },
  ]);
  const [rescuers, setRescuers] = useState<Rescuer[]>([]);



  // Funzione per avviare l'animazione
  function moveRescuerTo(id: number, to: Pos, seconds: number) {
    setRescuers((prev) =>
      prev.map((r) =>
        r.id === id ? { ...r, target: to } : r
      )
    );
    setAnimations((prev) => [
      ...prev.filter((a) => a.id !== id), // rimuovi eventuali animazioni precedenti per quell'id
      {
        id,
        from: rescuers.find((r) => r.id === id)?.pos ?? { x: 0, y: 0 },
        to,
        duration: seconds * 1000,
        startTime: performance.now(),
      },
    ]);
  }

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

      emergencies.forEach(({pos}) => {
        ctx.fillStyle = "#ef4444";
        ctx.fillRect(pos.x * CELL_SIZE, pos.y * CELL_SIZE, CELL_SIZE * 5, CELL_SIZE * 5);
      });

      ctx.fillStyle = "#3b82f6";
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
  // Sostituisci con l'IP corretto della tua macchina WSL se diverso
  const ws = new WebSocket("ws://172.28.236.125:8080");

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

          const colorMap: Record<string, string> = {
            ambulanza: "#3b82f6",
            carabinieri: "#5d2075",
            pompieri: "#f43f5e",
            polizia: "#10b981",
            guardia: "#22d3ee",
          };
          const color = colorMap[type] || "#22d3ee";

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
      }

    } catch (err) {
      console.error("❌ Errore parsing messaggio WebSocket:", event.data);
    }
  };

  return () => ws.close();
}, []);

  return (
    <div className="bg-gray-900 min-h-screen flex flex-col items-center justify-center text-white">
      <h1 className="text-2xl font-bold mb-4">🚨 Emergency Grid Monitor</h1>
      <button
        className="mb-4 px-4 py-2 bg-blue-600 rounded"
        onClick={() => moveRescuerTo(Math.floor(Math.random() * rescuers.length), { x: 100, y: 100 }, 10)}
      >
        Muovi un soccorritore
      </button>
      <button
        className="mb-4 px-4 py-2 bg-blue-600 rounded"
        onClick={() => setEmergencies((prev) => [...prev, { id: prev.length + 1, type: "bufera", pos: {x: Math.random() * GRID_WIDTH, y: Math.random() * GRID_HEIGHT }, status: "attivo" }])}
      >
        Genera un' emergenza
      </button>
      <button
        className="mb-4 px-4 py-2 bg-blue-600 rounded"
        onClick={() => setRescuers((prev) => [...prev, { id: prev.length + 1, type: "guardia", pos: { x: Math.random() * GRID_WIDTH, y: Math.random() * GRID_HEIGHT }}])}
      >
        Genera un' soccorritore
      </button>
      <div className="flex gap-10">
        <canvas
          ref={canvasRef}
          width={GRID_WIDTH * CELL_SIZE}
          height={GRID_HEIGHT * CELL_SIZE}
          className="border border-green-500 rounded bg-amber-50"
        />

        <div className="flex gap-3.5 mb-2">
          <div className="min-w-[350px]">
            <h1 className="text-2xl font-bold mb-4">🚑 Soccorritori</h1>
            <div className=" mb-2">
              <ul className="flex">
                <li className="w-1/12">ID</li>
                <li className="w-3/12">Tipo</li>
                <li className="w-4/12">Posizione</li>
                <li className="w-4/12">Stato</li>
              </ul>
            </div>
            <ul className="overflow-y-auto" style={{ maxHeight: `${GRID_HEIGHT * CELL_SIZE-90}px` }}>
              {rescuers.map((rescuer) => (
                <ul key={rescuer.id} className="mb-2 flex">
                  <li className="w-1/12">{rescuer.id}</li>
                  <li className="w-3/12">{rescuer.type}</li>
                  <li className="w-4/12"> ({rescuer.pos.x.toFixed(2)}, {rescuer.pos.y.toFixed(2)})</li>
                  <li className="w-4/12">{rescuer.status}</li>
                </ul>
              ))}
            </ul>
          </div>

          <div className="min-w-[350px]">
            <h1 className="text-2xl font-bold mb-4">🚨 Emergenze attive</h1>
            <div className=" mb-2">
              <ul className="flex">
                <li className="w-1/12">ID</li>
                <li className="w-3/12">Tipo</li>
                <li className="w-4/12">Posizione</li>
                <li className="w-4/12">Stato</li>
              </ul>
            </div>
            <ul className="overflow-y-auto" style={{ maxHeight: `${GRID_HEIGHT * CELL_SIZE-90}px` }}>
              {emergencies.map((emergency) => (
                <ul key={emergency.id} className="mb-2 flex">
                  <li className="w-1/12">{emergency.id}</li>
                  <li className="w-3/12">{emergency.type}</li>
                  <li className="w-4/12"> ({emergency.pos.x.toFixed(2)}, {emergency.pos.y.toFixed(2)})</li>
                  <li className="w-4/12">{emergency.status}</li>
                </ul>
              ))}
            </ul>
          </div>
        </div>
      </div>
    </div>
  );
}

export default App;
