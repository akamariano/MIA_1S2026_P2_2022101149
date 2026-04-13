"use client";
import { useState, useEffect } from "react";
import { getSession, SessionInfo } from "@/services/api";

interface SessionBarProps {
  onSessionChange?: () => void;
}

export default function SessionBar({ onSessionChange }: SessionBarProps) {
  const [session, setSession] = useState<SessionInfo>({
    active: false,
    username: "",
    groupname: "",
    partitionId: "",
  });

  const refresh = async () => {
    const s = await getSession();
    setSession(s);
    onSessionChange?.();
  };

  useEffect(() => {
    refresh();
    const interval = setInterval(refresh, 3000);
    return () => clearInterval(interval);
  }, []);

  if (!session.active) {
    return (
      <div className="flex items-center gap-3 bg-slate-800 border border-slate-700 rounded-xl px-4 py-3">
        <div className="w-2 h-2 rounded-full bg-red-500 flex-shrink-0" />
        <span className="text-slate-400 text-sm">Sin sesión activa</span>
        <span className="text-slate-500 text-xs ml-auto hidden sm:block">
          Usa el terminal para hacer login, o presiona &quot;Iniciar Sesión&quot;
        </span>
      </div>
    );
  }

  return (
    <div className="flex items-center gap-4 bg-slate-800 border border-green-800 rounded-xl px-4 py-3">
      <div className="w-2 h-2 rounded-full bg-green-400 animate-pulse flex-shrink-0" />
      <div className="flex flex-col min-w-0">
        <span className="text-green-300 font-semibold text-sm truncate">
          {session.username}
        </span>
        <span className="text-slate-400 text-xs truncate">
          Grupo: <span className="text-slate-300">{session.groupname}</span>
          &nbsp;·&nbsp;
          Partición: <span className="font-mono text-indigo-300">{session.partitionId}</span>
        </span>
      </div>
      <div className="ml-auto flex-shrink-0">
        <span className="text-xs px-2.5 py-1 bg-green-900/40 border border-green-800 text-green-400 rounded-full">
          Sesión activa
        </span>
      </div>
    </div>
  );
}
