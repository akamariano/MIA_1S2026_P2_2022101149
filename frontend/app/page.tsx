import CommandPanel from "@/components/CommandPanel";
import FileExplorer from "@/components/FileExplorer";

export default function Home() {
  return (
    <main className="min-h-screen bg-gradient-to-br from-slate-900 to-slate-800 p-8">
      <div className="max-w-6xl mx-auto space-y-8">
        <h1 className="text-3xl font-bold text-white">
          ExtreamFS Console
        </h1>

        {/* Terminal de comandos */}
        <CommandPanel />

        {/* Visualizador del sistema de archivos */}
        <FileExplorer />
      </div>
    </main>
  );
}