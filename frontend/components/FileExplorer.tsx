"use client";
import { useState, useEffect } from "react";
import {
  getDisks, browseDirectory, getFileContent, getJournaling,
  MountedDisk, FileEntry
} from "@/services/api";

export default function FileExplorer() {
  const [disks, setDisks] = useState<MountedDisk[]>([]);
  const [selectedDisk, setSelectedDisk] = useState<MountedDisk | null>(null);
  const [currentPath, setCurrentPath] = useState("/");
  const [entries, setEntries] = useState<FileEntry[]>([]);
  const [fileContent, setFileContent] = useState<string | null>(null);
  const [selectedFile, setSelectedFile] = useState<string | null>(null);
  const [journaling, setJournaling] = useState<string | null>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  // Cargar discos montados
  const loadDisks = async () => {
    const d = await getDisks();
    setDisks(d);
  };

  useEffect(() => { loadDisks(); }, []);

  // Seleccionar disco y navegar a raíz
  const selectDisk = async (disk: MountedDisk) => {
    setSelectedDisk(disk);
    setCurrentPath("/");
    setFileContent(null);
    setSelectedFile(null);
    setJournaling(null);
    await navigate(disk, "/");
  };

  // Navegar a carpeta
  const navigate = async (disk: MountedDisk, path: string) => {
    setLoading(true);
    setError(null);
    setFileContent(null);
    setSelectedFile(null);
    const result = await browseDirectory(disk.id, path);
    if (result) {
      setEntries(result.entries);
      setCurrentPath(path);
    } else {
      setError("No se pudo leer el directorio");
    }
    setLoading(false);
  };

  // Abrir archivo
  const openFile = async (entry: FileEntry) => {
    if (!selectedDisk) return;
    const filePath = currentPath === "/"
      ? "/" + entry.name
      : currentPath + "/" + entry.name;
    setLoading(true);
    const content = await getFileContent(selectedDisk.id, filePath);
    setSelectedFile(filePath);
    setFileContent(content);
    setLoading(false);
  };

  // Navegar a subcarpeta
  const openFolder = async (entry: FileEntry) => {
    if (!selectedDisk) return;
    const newPath = currentPath === "/"
      ? "/" + entry.name
      : currentPath + "/" + entry.name;
    await navigate(selectedDisk, newPath);
  };

  // Ir atrás
  const goBack = async () => {
    if (!selectedDisk || currentPath === "/") return;
    const parts = currentPath.split("/").filter(Boolean);
    parts.pop();
    const newPath = parts.length === 0 ? "/" : "/" + parts.join("/");
    await navigate(selectedDisk, newPath);
  };

  // Ver journaling
  const loadJournaling = async () => {
    if (!selectedDisk) return;
    setLoading(true);
    const j = await getJournaling(selectedDisk.id);
    setJournaling(j);
    setLoading(false);
  };

  const formatSize = (bytes: number) => {
    if (bytes < 1024) return `${bytes} B`;
    if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`;
    return `${(bytes / (1024 * 1024)).toFixed(1)} MB`;
  };

  return (
    <div className="bg-slate-800 rounded-xl p-6 text-white">
      <h2 className="text-2xl font-bold mb-4 text-indigo-400">
        📁 Explorador del Sistema de Archivos
      </h2>

      {/* Botón refrescar discos */}
      <button
        onClick={loadDisks}
        className="mb-4 px-4 py-2 bg-indigo-600 hover:bg-indigo-700 rounded-lg text-sm"
      >
        🔄 Refrescar Discos
      </button>

      {/* Lista de discos */}
      {disks.length === 0 ? (
        <p className="text-slate-400 text-sm">No hay particiones montadas.</p>
      ) : (
        <div className="grid grid-cols-2 md:grid-cols-4 gap-3 mb-6">
          {disks.map((disk) => (
            <button
              key={disk.id}
              onClick={() => selectDisk(disk)}
              className={`p-3 rounded-lg border text-left transition-all ${
                selectedDisk?.id === disk.id
                  ? "border-indigo-500 bg-indigo-900"
                  : "border-slate-600 bg-slate-700 hover:border-indigo-400"
              }`}
            >
              <div className="text-lg">💾</div>
              <div className="font-bold text-sm">{disk.id}</div>
              <div className="text-xs text-slate-400 truncate">{disk.name}</div>
              <div className="text-xs text-slate-500">{formatSize(disk.size)}</div>
            </button>
          ))}
        </div>
      )}

      {/* Explorador de archivos */}
      {selectedDisk && (
        <div className="space-y-4">
          {/* Barra de navegación */}
          <div className="flex items-center gap-2 bg-slate-700 rounded-lg p-2">
            <button
              onClick={goBack}
              disabled={currentPath === "/"}
              className="px-3 py-1 bg-slate-600 hover:bg-slate-500 rounded disabled:opacity-40 text-sm"
            >
              ← Atrás
            </button>
            <span className="text-slate-300 text-sm font-mono flex-1">
              {selectedDisk.id} : {currentPath}
            </span>
            <button
              onClick={loadJournaling}
              className="px-3 py-1 bg-purple-700 hover:bg-purple-600 rounded text-sm"
            >
              📋 Journal
            </button>
          </div>

          {/* Tabla de archivos */}
          {loading ? (
            <div className="text-center text-slate-400 py-8">Cargando...</div>
          ) : error ? (
            <div className="text-red-400 text-sm">{error}</div>
          ) : (
            <div className="overflow-x-auto">
              <table className="w-full text-sm">
                <thead>
                  <tr className="text-slate-400 border-b border-slate-600">
                    <th className="text-left py-2 px-3">Nombre</th>
                    <th className="text-left py-2 px-3">Permisos</th>
                    <th className="text-left py-2 px-3">Tamaño</th>
                    <th className="text-left py-2 px-3">Fecha</th>
                    <th className="text-left py-2 px-3">Tipo</th>
                  </tr>
                </thead>
                <tbody>
                  {entries.length === 0 && (
                    <tr>
                      <td colSpan={5} className="text-center text-slate-500 py-4">
                        Carpeta vacía
                      </td>
                    </tr>
                  )}
                  {entries.map((entry, i) => (
                    <tr
                      key={i}
                      onClick={() =>
                        entry.type === "folder"
                          ? openFolder(entry)
                          : openFile(entry)
                      }
                      className="border-b border-slate-700 hover:bg-slate-700 cursor-pointer transition-colors"
                    >
                      <td className="py-2 px-3">
                        <span className="mr-2">
                          {entry.type === "folder" ? "📁" : "📄"}
                        </span>
                        <span className={entry.type === "folder"
                          ? "text-yellow-300"
                          : "text-green-300"}>
                          {entry.name}
                        </span>
                      </td>
                      <td className="py-2 px-3 font-mono text-slate-300">
                        {entry.perm}
                      </td>
                      <td className="py-2 px-3 text-slate-400">
                        {entry.type === "file" ? formatSize(entry.size) : "-"}
                      </td>
                      <td className="py-2 px-3 text-slate-400">{entry.date}</td>
                      <td className="py-2 px-3 text-slate-400">
                        {entry.type === "folder" ? "Carpeta" : "Archivo"}
                      </td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          )}

          {/* Visor de archivo */}
          {fileContent !== null && (
            <div className="bg-slate-900 rounded-lg p-4">
              <div className="flex justify-between items-center mb-2">
                <span className="text-indigo-300 text-sm font-mono">
                  📄 {selectedFile}
                </span>
                <button
                  onClick={() => { setFileContent(null); setSelectedFile(null); }}
                  className="text-slate-400 hover:text-white text-sm"
                >
                  ✕ Cerrar
                </button>
              </div>
              <pre className="text-green-300 text-xs font-mono whitespace-pre-wrap overflow-auto max-h-64">
                {fileContent || "(archivo vacío)"}
              </pre>
            </div>
          )}

          {/* Visor de journaling */}
          {journaling !== null && (
            <div className="bg-slate-900 rounded-lg p-4">
              <div className="flex justify-between items-center mb-2">
                <span className="text-purple-300 text-sm font-bold">
                  📋 Journal — {selectedDisk.id}
                </span>
                <button
                  onClick={() => setJournaling(null)}
                  className="text-slate-400 hover:text-white text-sm"
                >
                  ✕ Cerrar
                </button>
              </div>
              <pre className="text-slate-300 text-xs font-mono whitespace-pre-wrap overflow-auto max-h-64">
                {journaling}
              </pre>
            </div>
          )}
        </div>
      )}
    </div>
  );
}