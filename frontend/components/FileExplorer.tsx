"use client";
import { useState, useEffect } from "react";
import {
  getDisks, browseDirectory, getFileContent, getJournaling,
  MountedDisk, FileEntry
} from "@/services/api";

// ── Disk group (multiple partitions can share the same .mia file) ─────────────
interface DiskGroup {
  path: string;
  displayName: string;
  partitions: MountedDisk[];
}

type Step = "disk" | "partition" | "browse" | "file";

interface FileExplorerProps {
  onReady?: (refreshFn: () => void) => void;
}

// ── SVG icons ─────────────────────────────────────────────────────────────────
function HardDriveIcon() {
  return (
    <svg viewBox="0 0 80 80" className="w-16 h-16" fill="none" xmlns="http://www.w3.org/2000/svg">
      <rect x="6" y="16" width="68" height="48" rx="10" fill="#1e293b" stroke="#475569" strokeWidth="2"/>
      <rect x="6" y="40" width="68" height="24" rx="6" fill="#0f172a" stroke="#475569" strokeWidth="1.5"/>
      <circle cx="60" cy="52" r="5" fill="#4f46e5"/>
      <circle cx="47" cy="52" r="4" fill="#334155" stroke="#64748b" strokeWidth="1"/>
      <rect x="14" y="26" width="28" height="5" rx="2.5" fill="#334155"/>
      <rect x="14" y="34" width="18" height="3" rx="1.5" fill="#1e3a5f"/>
      <rect x="14" y="48" width="14" height="3" rx="1.5" fill="#334155"/>
    </svg>
  );
}

function PartitionIcon({ index }: { index: number }) {
  const colors = ["#4f46e5", "#0891b2", "#059669", "#d97706"];
  const color = colors[index % colors.length];
  return (
    <svg viewBox="0 0 80 80" className="w-14 h-14" fill="none" xmlns="http://www.w3.org/2000/svg">
      <circle cx="40" cy="40" r="32" fill="#0f172a" stroke={color} strokeWidth="2.5"/>
      <circle cx="40" cy="40" r="22" fill="#1e293b" stroke={color} strokeWidth="1.5"/>
      <circle cx="40" cy="40" r="10" fill="#334155" stroke={color} strokeWidth="1"/>
      <circle cx="40" cy="40" r="4" fill={color}/>
      <rect x="38" y="8" width="4" height="10" rx="2" fill={color}/>
      <rect x="38" y="62" width="4" height="10" rx="2" fill={color}/>
    </svg>
  );
}

function FolderIcon() {
  return (
    <svg viewBox="0 0 80 80" className="w-14 h-14" fill="none" xmlns="http://www.w3.org/2000/svg">
      <path d="M6 22 C6 18 9 16 12 16 L30 16 L36 24 L68 24 C71 24 74 27 74 30 L74 62 C74 65 71 68 68 68 L12 68 C9 68 6 65 6 62 Z" fill="#92400e"/>
      <path d="M6 30 L74 30 L74 62 C74 65 71 68 68 68 L12 68 C9 68 6 65 6 62 Z" fill="#f59e0b"/>
      <path d="M6 30 L74 30 L74 38 L6 38 Z" fill="#fbbf24" fillOpacity="0.4"/>
    </svg>
  );
}

function FileIcon() {
  return (
    <svg viewBox="0 0 80 80" className="w-14 h-14" fill="none" xmlns="http://www.w3.org/2000/svg">
      <path d="M14 8 L50 8 L66 24 L66 72 L14 72 Z" fill="#1e3a5f" stroke="#3b82f6" strokeWidth="1.5"/>
      <path d="M50 8 L50 24 L66 24 Z" fill="#3b82f6" fillOpacity="0.6"/>
      <rect x="22" y="36" width="36" height="3" rx="1.5" fill="#93c5fd"/>
      <rect x="22" y="43" width="30" height="3" rx="1.5" fill="#93c5fd"/>
      <rect x="22" y="50" width="33" height="3" rx="1.5" fill="#93c5fd"/>
      <rect x="22" y="57" width="26" height="3" rx="1.5" fill="#93c5fd" fillOpacity="0.6"/>
    </svg>
  );
}

// ── Helper ────────────────────────────────────────────────────────────────────
function formatSize(bytes: number) {
  if (bytes < 1024) return `${bytes} B`;
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`;
  return `${(bytes / 1024 / 1024).toFixed(1)} MB`;
}

// ── Component ─────────────────────────────────────────────────────────────────
export default function FileExplorer({ onReady }: FileExplorerProps) {
  const [diskGroups, setDiskGroups] = useState<DiskGroup[]>([]);
  const [step, setStep] = useState<Step>("disk");
  const [selectedGroup, setSelectedGroup] = useState<DiskGroup | null>(null);
  const [selectedPartition, setSelectedPartition] = useState<MountedDisk | null>(null);
  const [currentPath, setCurrentPath] = useState("/");
  const [entries, setEntries] = useState<FileEntry[]>([]);
  const [fileContent, setFileContent] = useState<string | null>(null);
  const [selectedFile, setSelectedFile] = useState<string | null>(null);
  const [journaling, setJournaling] = useState<string | null>(null);
  const [showJournal, setShowJournal] = useState(false);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const loadDisks = async () => {
    const disks = await getDisks();
    const groups: Record<string, DiskGroup> = {};
    for (const d of disks) {
      if (!groups[d.path]) {
        const parts = d.path.split("/");
        groups[d.path] = {
          path: d.path,
          displayName: parts[parts.length - 1],
          partitions: [],
        };
      }
      groups[d.path].partitions.push(d);
    }
    setDiskGroups(Object.values(groups));
  };

  useEffect(() => {
    loadDisks();
    onReady?.(loadDisks);
  }, []);

  // ── Navigation ──────────────────────────────────────────────────────────────
  const selectGroup = (group: DiskGroup) => {
    setSelectedGroup(group);
    setStep("partition");
    setShowJournal(false);
    setJournaling(null);
  };

  const selectPartition = async (partition: MountedDisk) => {
    setSelectedPartition(partition);
    setStep("browse");
    setCurrentPath("/");
    setFileContent(null);
    setSelectedFile(null);
    setShowJournal(false);
    setJournaling(null);
    await navigateTo(partition, "/");
  };

  const navigateTo = async (partition: MountedDisk, path: string) => {
    setLoading(true);
    setError(null);
    setFileContent(null);
    setSelectedFile(null);
    setShowJournal(false);
    const result = await browseDirectory(partition.id, path);
    if (result) {
      setEntries(result.entries);
      setCurrentPath(path);
      setStep("browse");
    } else {
      setError("No se pudo leer el directorio");
    }
    setLoading(false);
  };

  const openFile = async (entry: FileEntry) => {
    if (!selectedPartition) return;
    const filePath = currentPath === "/" ? `/${entry.name}` : `${currentPath}/${entry.name}`;
    setLoading(true);
    const content = await getFileContent(selectedPartition.id, filePath);
    setSelectedFile(filePath);
    setFileContent(content);
    setStep("file");
    setLoading(false);
  };

  const openFolder = (entry: FileEntry) => {
    if (!selectedPartition) return;
    const newPath = currentPath === "/" ? `/${entry.name}` : `${currentPath}/${entry.name}`;
    navigateTo(selectedPartition, newPath);
  };

  const goUp = () => {
    if (!selectedPartition || currentPath === "/") return;
    const parts = currentPath.split("/").filter(Boolean);
    parts.pop();
    navigateTo(selectedPartition, parts.length === 0 ? "/" : "/" + parts.join("/"));
  };

  const loadJournal = async () => {
    if (!selectedPartition) return;
    if (showJournal) { setShowJournal(false); return; }
    setLoading(true);
    const j = await getJournaling(selectedPartition.id);
    setJournaling(j);
    setShowJournal(true);
    setLoading(false);
  };

  // ── Breadcrumb ──────────────────────────────────────────────────────────────
  const goToPathSegment = (index: number) => {
    if (!selectedPartition) return;
    const parts = currentPath.split("/").filter(Boolean);
    navigateTo(selectedPartition, index < 0 ? "/" : "/" + parts.slice(0, index + 1).join("/"));
  };

  const pathParts = currentPath === "/" ? [] : currentPath.split("/").filter(Boolean);

  // ── Render ──────────────────────────────────────────────────────────────────
  return (
    <div className="bg-slate-800 rounded-xl overflow-hidden border border-slate-700">

      {/* ── Header ─────────────────────────────────────────────────────────── */}
      <div className="bg-slate-900 px-6 py-4 border-b border-slate-700">
        <div className="flex items-center justify-between">
          <h2 className="text-lg font-semibold text-white">
            Visualizador del Sistema de Archivos
          </h2>
          <button
            onClick={loadDisks}
            className="text-xs px-3 py-1.5 bg-slate-700 hover:bg-slate-600 rounded-lg text-slate-300 transition-colors"
          >
            Refrescar
          </button>
        </div>

        {/* Breadcrumb */}
        <div className="flex items-center gap-1 mt-2 flex-wrap">
          <button
            onClick={() => { setStep("disk"); setSelectedGroup(null); setSelectedPartition(null); }}
            className="text-xs text-indigo-400 hover:text-indigo-300 font-medium"
          >
            Discos
          </button>
          {selectedGroup && (
            <>
              <span className="text-slate-600 text-xs">/</span>
              <button
                onClick={() => { setStep("partition"); setSelectedPartition(null); }}
                className="text-xs text-indigo-400 hover:text-indigo-300 font-medium"
              >
                {selectedGroup.displayName}
              </button>
            </>
          )}
          {selectedPartition && (
            <>
              <span className="text-slate-600 text-xs">/</span>
              <button
                onClick={() => selectedPartition && navigateTo(selectedPartition, "/")}
                className="text-xs text-indigo-400 hover:text-indigo-300 font-medium"
              >
                {selectedPartition.name}
              </button>
            </>
          )}
          {(step === "browse" || step === "file") && pathParts.map((seg, i) => (
            <span key={i} className="flex items-center gap-1">
              <span className="text-slate-600 text-xs">/</span>
              <button
                onClick={() => step === "browse" ? goToPathSegment(i) : undefined}
                className={`text-xs font-medium ${
                  i === pathParts.length - 1 && step === "browse"
                    ? "text-white"
                    : "text-indigo-400 hover:text-indigo-300"
                }`}
              >
                {seg}
              </button>
            </span>
          ))}
          {step === "file" && selectedFile && (
            <>
              <span className="text-slate-600 text-xs">/</span>
              <span className="text-xs text-white font-medium">
                {selectedFile.split("/").pop()}
              </span>
            </>
          )}
        </div>
      </div>

      {/* ── Body ───────────────────────────────────────────────────────────── */}
      <div className="p-6">

        {/* ── STEP: Disk Selection ─────────────────────────────────────────── */}
        {step === "disk" && (
          <>
            {diskGroups.length === 0 ? (
              <div className="text-center py-16">
                <div className="flex justify-center mb-4 opacity-30">
                  <HardDriveIcon />
                </div>
                <p className="text-slate-400 font-medium">No hay particiones montadas</p>
                <p className="text-slate-500 text-sm mt-1">
                  Usa el terminal para crear discos, particiones y montarlas.
                </p>
              </div>
            ) : (
              <>
                <p className="text-slate-400 text-sm mb-5">
                  Seleccione el disco que desea visualizar:
                </p>
                <div className="grid grid-cols-2 sm:grid-cols-3 md:grid-cols-4 lg:grid-cols-5 gap-4">
                  {diskGroups.map((group) => (
                    <button
                      key={group.path}
                      onClick={() => selectGroup(group)}
                      className="flex flex-col items-center gap-3 p-5 rounded-xl border border-slate-600
                                 bg-slate-700/40 hover:bg-slate-700 hover:border-indigo-500
                                 transition-all group cursor-pointer"
                    >
                      <HardDriveIcon />
                      <div className="text-center">
                        <p className="font-semibold text-sm text-white group-hover:text-indigo-300 transition-colors">
                          {group.displayName}
                        </p>
                        <p className="text-xs text-slate-400 mt-0.5">
                          {group.partitions.length} partición{group.partitions.length !== 1 ? "es" : ""}
                        </p>
                        <p className="text-xs text-slate-500">
                          {formatSize(group.partitions.reduce((a, p) => a + p.size, 0))}
                        </p>
                      </div>
                    </button>
                  ))}
                </div>
              </>
            )}
          </>
        )}

        {/* ── STEP: Partition Selection ─────────────────────────────────────── */}
        {step === "partition" && selectedGroup && (
          <>
            <div className="flex items-center gap-3 mb-5">
              <button
                onClick={() => { setStep("disk"); setSelectedGroup(null); }}
                className="text-sm text-slate-400 hover:text-white transition-colors flex items-center gap-1"
              >
                ← Volver
              </button>
              <span className="text-slate-600">|</span>
              <p className="text-slate-400 text-sm">
                Seleccione la partición que desea visualizar:
              </p>
            </div>
            <div className="grid grid-cols-2 sm:grid-cols-3 md:grid-cols-4 gap-4">
              {selectedGroup.partitions.map((partition, idx) => (
                <button
                  key={partition.id}
                  onClick={() => selectPartition(partition)}
                  className="flex flex-col items-center gap-3 p-5 rounded-xl border border-slate-600
                             bg-slate-700/40 hover:bg-slate-700 hover:border-indigo-500
                             transition-all group cursor-pointer"
                >
                  <PartitionIcon index={idx} />
                  <div className="text-center">
                    <p className="font-semibold text-sm text-white group-hover:text-indigo-300 transition-colors">
                      {partition.name}
                    </p>
                    <p className="text-xs font-mono text-indigo-400 mt-0.5">{partition.id}</p>
                    <p className="text-xs text-slate-400 mt-0.5">{formatSize(partition.size)}</p>
                  </div>
                </button>
              ))}
            </div>
          </>
        )}

        {/* ── STEP: File Browse ─────────────────────────────────────────────── */}
        {step === "browse" && selectedPartition && (
          <>
            {/* Path bar */}
            <div className="flex items-center gap-2 bg-slate-900 rounded-lg px-4 py-2.5 mb-4">
              <button
                onClick={() => {
                  if (currentPath === "/") {
                    setStep("partition");
                    setSelectedPartition(null);
                  } else {
                    goUp();
                  }
                }}
                className="text-slate-400 hover:text-white text-sm transition-colors"
                title="Volver"
              >
                ←
              </button>
              <span className="text-slate-300 font-mono text-sm flex-1 select-all">
                {currentPath}
              </span>
              <button
                onClick={loadJournal}
                className="text-xs px-2.5 py-1 bg-purple-900/60 hover:bg-purple-800 border border-purple-700
                           rounded text-purple-300 transition-colors"
              >
                Journal
              </button>
            </div>

            {loading ? (
              <div className="text-center py-16 text-slate-400">Cargando...</div>
            ) : error ? (
              <div className="text-red-400 text-sm px-2">{error}</div>
            ) : (
              <>
                <p className="text-slate-500 text-xs mb-4">
                  Navegue entre carpetas o visualice archivos
                </p>
                {entries.length === 0 ? (
                  <div className="text-center py-10 text-slate-500 text-sm">
                    Carpeta vacía
                  </div>
                ) : (
                  <div className="grid grid-cols-3 sm:grid-cols-4 md:grid-cols-5 lg:grid-cols-6 gap-3">
                    {entries.map((entry, i) => (
                      <button
                        key={i}
                        onClick={() => entry.type === "folder" ? openFolder(entry) : openFile(entry)}
                        className="flex flex-col items-center gap-2 p-3 rounded-xl
                                   hover:bg-slate-700 transition-all group text-center"
                        title={`${entry.name} — ${entry.perm} — ${entry.type === "file" ? formatSize(entry.size) : "carpeta"}`}
                      >
                        {entry.type === "folder" ? <FolderIcon /> : <FileIcon />}
                        <div>
                          <p className="text-xs font-medium text-slate-200 group-hover:text-white
                                        break-all leading-tight line-clamp-2">
                            {entry.name}
                          </p>
                          <p className="text-xs text-slate-500 font-mono mt-0.5">{entry.perm}</p>
                          {entry.type === "file" && (
                            <p className="text-xs text-slate-600 mt-0.5">{formatSize(entry.size)}</p>
                          )}
                        </div>
                      </button>
                    ))}
                  </div>
                )}
              </>
            )}

            {/* Journal panel */}
            {showJournal && journaling !== null && (
              <div className="mt-5 bg-slate-900 rounded-xl border border-purple-800 p-4">
                <div className="flex justify-between items-center mb-3">
                  <span className="text-purple-300 text-sm font-semibold">
                    Journal — {selectedPartition.name} ({selectedPartition.id})
                  </span>
                  <button
                    onClick={() => setShowJournal(false)}
                    className="text-slate-400 hover:text-white text-sm"
                  >
                    ✕
                  </button>
                </div>
                <pre className="text-slate-300 text-xs font-mono whitespace-pre-wrap
                                overflow-auto max-h-72">
                  {journaling}
                </pre>
              </div>
            )}
          </>
        )}

        {/* ── STEP: File Viewer ─────────────────────────────────────────────── */}
        {step === "file" && selectedFile && (
          <>
            {/* Path bar */}
            <div className="flex items-center gap-2 bg-slate-900 rounded-lg px-4 py-2.5 mb-4">
              <button
                onClick={() => {
                  setStep("browse");
                  setFileContent(null);
                  setSelectedFile(null);
                }}
                className="text-slate-400 hover:text-white text-sm transition-colors"
              >
                ←
              </button>
              <span className="text-slate-300 font-mono text-sm flex-1 select-all">
                {selectedFile}
              </span>
            </div>

            <p className="text-slate-500 text-xs mb-3">Contenido del archivo</p>

            {loading ? (
              <div className="text-center py-8 text-slate-400">Cargando...</div>
            ) : (
              <div className="bg-slate-900 rounded-xl border border-slate-700 p-4">
                <pre className="text-green-300 text-sm font-mono whitespace-pre-wrap
                                overflow-auto max-h-96 min-h-24 leading-relaxed">
                  {fileContent ?? "(archivo vacío)"}
                </pre>
              </div>
            )}
          </>
        )}
      </div>
    </div>
  );
}
