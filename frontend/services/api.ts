const API_URL = "http://localhost:8080";

export async function executeCommand(command: string): Promise<string> {
  try {
    const res = await fetch(`${API_URL}/command`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ command }),
    });
    const data = await res.json();
    return data.output ?? data.error ?? "Sin respuesta";
  } catch (err) {
    return "ERROR: No se pudo conectar al backend";
  }
}

export async function getReport(path: string): Promise<string> {
  return `${API_URL}/report?path=${encodeURIComponent(path)}`;
}

export async function checkStatus(): Promise<boolean> {
  try {
    const res = await fetch(`${API_URL}/status`);
    const data = await res.json();
    return data.status === "ok";
  } catch {
    return false;
  }
}

export interface MountedDisk {
  id: string;
  path: string;
  name: string;
  start: number;
  size: number;
}

export interface FileEntry {
  name: string;
  type: "folder" | "file";
  size: number;
  perm: string;
  uid: number;
  gid: number;
  date: string;
}

export interface BrowseResult {
  path: string;
  entries: FileEntry[];
}

export async function getDisks(): Promise<MountedDisk[]> {
  try {
    const res = await fetch(`${API_URL}/disks`);
    return await res.json();
  } catch {
    return [];
  }
}

export async function browseDirectory(id: string, path: string): Promise<BrowseResult | null> {
  try {
    const res = await fetch(
      `${API_URL}/browse?id=${encodeURIComponent(id)}&path=${encodeURIComponent(path)}`
    );
    if (!res.ok) return null;
    return await res.json();
  } catch {
    return null;
  }
}

export async function getFileContent(id: string, path: string): Promise<string | null> {
  try {
    const res = await fetch(
      `${API_URL}/file?id=${encodeURIComponent(id)}&path=${encodeURIComponent(path)}`
    );
    if (!res.ok) return null;
    const data = await res.json();
    return data.content ?? null;
  } catch {
    return null;
  }
}

export async function getJournaling(id: string): Promise<string> {
  try {
    const res = await fetch(`${API_URL}/journaling?id=${encodeURIComponent(id)}`);
    const data = await res.json();
    return data.output ?? "Sin entradas";
  } catch {
    return "ERROR: No se pudo obtener journaling";
  }
}