"use client";
import { useState, useEffect } from "react";
import { useRouter } from "next/navigation";
import { executeCommand, getDisks, getSession, MountedDisk } from "@/services/api";

export default function LoginPage() {
  const router = useRouter();
  const [partitionId, setPartitionId] = useState("");
  const [username, setUsername] = useState("");
  const [password, setPassword] = useState("");
  const [remember, setRemember] = useState(false);
  const [error, setError] = useState("");
  const [loading, setLoading] = useState(false);
  const [disks, setDisks] = useState<MountedDisk[]>([]);

  useEffect(() => {
    // Si ya hay sesión activa, redirigir a home
    getSession().then((s) => {
      if (s.active) router.push("/");
    });
    // Cargar particiones montadas para sugerir ID
    getDisks().then(setDisks);
    // Precargar si hay algo guardado
    const saved = localStorage.getItem("lastPartitionId");
    if (saved) setPartitionId(saved);
  }, []);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!partitionId || !username || !password) {
      setError("Todos los campos son obligatorios");
      return;
    }
    setLoading(true);
    setError("");

    const cmd = `login -user=${username} -pass=${password} -id=${partitionId}`;
    const result = await executeCommand(cmd);

    if (result.includes("ERROR") || result.includes("error")) {
      setError(result.trim());
    } else {
      if (remember) {
        localStorage.setItem("lastPartitionId", partitionId);
      } else {
        localStorage.removeItem("lastPartitionId");
      }
      router.push("/");
    }
    setLoading(false);
  };

  return (
    <main className="min-h-screen bg-gradient-to-br from-slate-900 to-slate-800 flex items-center justify-center p-4">
      <div className="w-full max-w-md">
        {/* Header */}
        <div className="text-center mb-8">
          <div className="text-5xl mb-3">🗄️</div>
          <h1 className="text-3xl font-bold text-white">ExtreamFS</h1>
          <p className="text-slate-400 text-sm mt-1">Sistema de Archivos EXT2/EXT3</p>
        </div>

        {/* Card */}
        <div className="bg-slate-800 border border-slate-700 rounded-2xl p-8 shadow-2xl">
          <h2 className="text-xl font-semibold text-white mb-6 text-center">
            Iniciar Sesión
          </h2>

          <form onSubmit={handleSubmit} className="space-y-4">
            {/* ID Partición */}
            <div>
              <label className="block text-sm font-medium text-slate-300 mb-1">
                ID Partición
              </label>
              <input
                type="text"
                value={partitionId}
                onChange={(e) => setPartitionId(e.target.value.toUpperCase())}
                placeholder="Ej: 491A"
                className="w-full bg-slate-700 border border-slate-600 text-white rounded-lg px-3 py-2.5
                           placeholder-slate-500 focus:outline-none focus:ring-2 focus:ring-indigo-500
                           font-mono text-sm"
              />
              {disks.length > 0 && (
                <div className="mt-1 flex flex-wrap gap-1">
                  {disks.map((d) => (
                    <button
                      key={d.id}
                      type="button"
                      onClick={() => setPartitionId(d.id)}
                      className="text-xs px-2 py-0.5 bg-slate-600 hover:bg-indigo-700 text-slate-300
                                 rounded transition-colors font-mono"
                    >
                      {d.id} ({d.name})
                    </button>
                  ))}
                </div>
              )}
            </div>

            {/* Usuario */}
            <div>
              <label className="block text-sm font-medium text-slate-300 mb-1">
                Usuario
              </label>
              <input
                type="text"
                value={username}
                onChange={(e) => setUsername(e.target.value)}
                placeholder="root"
                className="w-full bg-slate-700 border border-slate-600 text-white rounded-lg px-3 py-2.5
                           placeholder-slate-500 focus:outline-none focus:ring-2 focus:ring-indigo-500 text-sm"
              />
            </div>

            {/* Contraseña */}
            <div>
              <label className="block text-sm font-medium text-slate-300 mb-1">
                Contraseña
              </label>
              <input
                type="password"
                value={password}
                onChange={(e) => setPassword(e.target.value)}
                placeholder="••••••"
                className="w-full bg-slate-700 border border-slate-600 text-white rounded-lg px-3 py-2.5
                           placeholder-slate-500 focus:outline-none focus:ring-2 focus:ring-indigo-500 text-sm"
              />
            </div>

            {/* Recordar */}
            <div className="flex items-center gap-2">
              <input
                id="remember"
                type="checkbox"
                checked={remember}
                onChange={(e) => setRemember(e.target.checked)}
                className="w-4 h-4 accent-indigo-500"
              />
              <label htmlFor="remember" className="text-sm text-slate-400">
                Recordar ID de partición
              </label>
            </div>

            {/* Error */}
            {error && (
              <div className="bg-red-900/40 border border-red-700 text-red-300 text-sm rounded-lg px-3 py-2">
                {error}
              </div>
            )}

            {/* Submit */}
            <button
              type="submit"
              disabled={loading}
              className="w-full bg-indigo-600 hover:bg-indigo-700 disabled:opacity-50
                         text-white font-semibold py-2.5 rounded-lg transition-colors"
            >
              {loading ? "Iniciando sesión..." : "Iniciar Sesión"}
            </button>
          </form>
        </div>

        {/* Volver */}
        <div className="text-center mt-4">
          <button
            onClick={() => router.push("/")}
            className="text-slate-500 hover:text-slate-300 text-sm transition-colors"
          >
            ← Volver al terminal
          </button>
        </div>
      </div>
    </main>
  );
}
