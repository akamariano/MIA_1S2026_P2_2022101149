"use client";
import { useRef } from "react";
import CommandPanel from "@/components/CommandPanel";
import FileExplorer from "@/components/FileExplorer";

export default function Home() {
  // Ref para llamar loadDisks desde FileExplorer
  const refreshDisksRef = useRef<(() => void) | null>(null);

  return (
    <main className="min-h-screen bg-gradient-to-br from-slate-900 to-slate-800 p-8">
      <div className="max-w-6xl mx-auto space-y-8">
        <h1 className="text-3xl font-bold text-white">
          ExtreamFS Console
        </h1>

        {/* Terminal de comandos — refresca el explorador al terminar */}
        <CommandPanel onScriptFinished={() => refreshDisksRef.current?.()} />

        {/* Explorador — expone su función loadDisks al padre */}
        <FileExplorer onReady={(fn) => { refreshDisksRef.current = fn; }} />
      </div>
    </main>
  );
}