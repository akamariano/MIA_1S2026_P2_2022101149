"use client";
import { useRef, useState, useEffect } from "react";
import { useRouter } from "next/navigation";
import CommandPanel from "@/components/CommandPanel";
import FileExplorer from "@/components/FileExplorer";
import SessionBar from "@/components/SessionBar";
import { getSession, executeCommand } from "@/services/api";

export default function Home() {
  const router = useRouter();
  const refreshDisksRef = useRef<(() => void) | null>(null);
  const [sessionActive, setSessionActive] = useState(false);
  const [sessionKey, setSessionKey] = useState(0);

  const refreshSession = async () => {
    const s = await getSession();
    setSessionActive(s.active);
  };

  useEffect(() => {
    refreshSession();
    const interval = setInterval(refreshSession, 3000);
    return () => clearInterval(interval);
  }, []);

  const handleLogout = async () => {
    await executeCommand("logout");
    await refreshSession();
    setSessionKey((k) => k + 1);
    refreshDisksRef.current?.();
  };

  const handleSessionChange = () => {
    refreshSession();
    setSessionKey((k) => k + 1);
    refreshDisksRef.current?.();
  };

  return (
    <main className="min-h-screen bg-gradient-to-br from-slate-900 to-slate-800 p-6">
      <div className="max-w-6xl mx-auto space-y-6">

        {/* Header */}
        <div className="flex items-center justify-between">
          <div>
            <h1 className="text-3xl font-bold text-white tracking-tight">
              ExtreamFS
            </h1>
            <p className="text-slate-500 text-sm mt-0.5">
              Sistema de Archivos EXT2/EXT3 — MIA Proyecto 2
            </p>
          </div>

          {sessionActive ? (
            <button
              onClick={handleLogout}
              className="px-4 py-2 bg-red-600 hover:bg-red-700 text-white text-sm
                         font-medium rounded-lg transition-colors"
            >
              Cerrar Sesión
            </button>
          ) : (
            <button
              onClick={() => router.push("/login")}
              className="px-4 py-2 bg-indigo-600 hover:bg-indigo-700 text-white text-sm
                         font-medium rounded-lg transition-colors"
            >
              Iniciar Sesión
            </button>
          )}
        </div>

        {/* Barra de sesión activa */}
        <SessionBar key={sessionKey} onSessionChange={handleSessionChange} />

        {/* Terminal de comandos */}
        <CommandPanel
          onScriptFinished={() => {
            refreshDisksRef.current?.();
            handleSessionChange();
          }}
        />

        {/* Visualizador del sistema de archivos */}
        <FileExplorer onReady={(fn) => { refreshDisksRef.current = fn; }} />

      </div>
    </main>
  );
}
