import { useState, useRef, type FormEvent } from "react";
import { useQuery, useMutation, useQueryClient } from "@tanstack/react-query";
import * as firmwareApi from "../api/firmware";
import ConfirmModal from "../components/ConfirmModal";
import { useToast } from "../hooks/useToast";
import type { FirmwareInfo } from "../types";

function formatSize(bytes: number): string {
  if (bytes < 1024) return `${bytes} B`;
  const kb = bytes / 1024;
  if (kb < 1024) return `${kb.toFixed(1)} KB`;
  return `${(kb / 1024).toFixed(2)} MB`;
}

function formatDate(iso: string): string {
  return new Date(iso).toLocaleString("pt-BR", {
    day: "2-digit",
    month: "2-digit",
    year: "numeric",
    hour: "2-digit",
    minute: "2-digit",
  });
}

export default function Firmware() {
  const queryClient = useQueryClient();
  const toast = useToast();
  const fileRef = useRef<HTMLInputElement>(null);
  const [version, setVersion] = useState("");
  const [file, setFile] = useState<File | null>(null);
  const [showForm, setShowForm] = useState(false);
  const [deleteTarget, setDeleteTarget] = useState<FirmwareInfo | null>(null);

  const { data: firmwares = [], isLoading } = useQuery({
    queryKey: ["firmwares"],
    queryFn: firmwareApi.listFirmwares,
  });

  const uploadMutation = useMutation({
    mutationFn: () => firmwareApi.uploadFirmware(version, file!),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["firmwares"] });
      setVersion("");
      setFile(null);
      setShowForm(false);
      if (fileRef.current) fileRef.current.value = "";
      toast.success("Firmware enviado! Dispositivos serao atualizados automaticamente.");
    },
    onError: () => toast.error("Erro ao enviar firmware"),
  });

  const deleteMutation = useMutation({
    mutationFn: (id: string) => firmwareApi.deleteFirmware(id),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["firmwares"] });
      toast.success("Firmware removido");
    },
    onError: () => toast.error("Erro ao remover firmware"),
  });

  function handleUpload(e: FormEvent) {
    e.preventDefault();
    if (version.trim() && file) uploadMutation.mutate();
  }

  if (isLoading) {
    return <p className="text-gray-500 dark:text-slate-400">Carregando...</p>;
  }

  return (
    <div>
      <div className="flex items-center justify-between">
        <h1 className="text-2xl font-bold text-gray-900 dark:text-white">Firmware OTA</h1>
        {!showForm && (
          <button
            onClick={() => setShowForm(true)}
            className="rounded-lg bg-indigo-600 dark:bg-indigo-500 px-4 py-2 text-sm font-semibold text-white hover:bg-indigo-700 dark:hover:bg-indigo-600 transition-colors"
          >
            Enviar firmware
          </button>
        )}
      </div>

      <div className="mt-2 flex items-start gap-2 rounded-lg border border-blue-200 dark:border-blue-800/50 bg-blue-50 dark:bg-blue-900/20 px-4 py-3">
        <span className="text-blue-500 mt-0.5 text-sm">&#9432;</span>
        <p className="text-xs text-blue-700 dark:text-blue-300">
          Ao enviar um novo firmware, todos os dispositivos conectados serao atualizados
          automaticamente no proximo ciclo de polling (~60s).
        </p>
      </div>

      {showForm && (
        <form
          onSubmit={handleUpload}
          className="mt-4 rounded-2xl bg-white dark:bg-slate-900 border border-gray-100 dark:border-slate-800 p-6 shadow-sm dark:shadow-black/10"
        >
          <h2 className="text-lg font-semibold text-gray-900 dark:text-white mb-4">
            Novo firmware
          </h2>
          <div className="space-y-4 max-w-md">
            <div>
              <label
                htmlFor="fw-version"
                className="block text-sm font-medium text-gray-700 dark:text-slate-300"
              >
                Versao
              </label>
              <input
                id="fw-version"
                required
                value={version}
                onChange={(e) => setVersion(e.target.value)}
                placeholder="Ex: 1.2.0"
                pattern="[\w.\-]+"
                className="mt-1 block w-full rounded border border-gray-300 dark:border-slate-600 dark:bg-slate-800 dark:text-white px-3 py-2 text-sm focus:border-indigo-500 focus:ring-1 focus:ring-indigo-500 focus:outline-none"
              />
            </div>
            <div>
              <label
                htmlFor="fw-file"
                className="block text-sm font-medium text-gray-700 dark:text-slate-300"
              >
                Arquivo .bin
              </label>
              <input
                id="fw-file"
                ref={fileRef}
                type="file"
                required
                accept=".bin"
                onChange={(e) => setFile(e.target.files?.[0] ?? null)}
                className="mt-1 block w-full text-sm text-gray-700 dark:text-slate-300 file:mr-3 file:rounded file:border-0 file:bg-indigo-50 dark:file:bg-indigo-900/30 file:px-3 file:py-2 file:text-sm file:font-medium file:text-indigo-600 dark:file:text-indigo-400 hover:file:bg-indigo-100 dark:hover:file:bg-indigo-900/50"
              />
              {file && (
                <p className="mt-1 text-xs text-gray-400 dark:text-slate-500">
                  {file.name} — {formatSize(file.size)}
                </p>
              )}
            </div>
            <div className="flex gap-2">
              <button
                type="submit"
                disabled={uploadMutation.isPending || !file}
                className="rounded-lg bg-indigo-600 dark:bg-indigo-500 px-4 py-2 text-sm font-semibold text-white hover:bg-indigo-700 dark:hover:bg-indigo-600 disabled:opacity-50 transition-colors"
              >
                {uploadMutation.isPending ? "Enviando..." : "Enviar"}
              </button>
              <button
                type="button"
                onClick={() => {
                  setShowForm(false);
                  setFile(null);
                  setVersion("");
                }}
                className="rounded border border-gray-300 dark:border-slate-600 px-4 py-2 text-sm text-gray-700 dark:text-slate-300 hover:bg-gray-50 dark:hover:bg-slate-800 transition-colors"
              >
                Cancelar
              </button>
            </div>
          </div>
        </form>
      )}

      {firmwares.length === 0 && !showForm ? (
        <p className="mt-6 text-center text-gray-500 dark:text-slate-400">
          Nenhum firmware enviado ainda.
        </p>
      ) : (
        <div className="mt-4 space-y-3">
          {firmwares.map((fw, i) => (
            <div
              key={fw.id}
              className="flex items-center justify-between rounded-xl bg-white dark:bg-slate-900 p-4 shadow-sm dark:shadow-black/10 border border-gray-100 dark:border-slate-800"
            >
              <div className="min-w-0 flex-1">
                <div className="flex items-center gap-2">
                  <h3 className="font-medium text-gray-900 dark:text-white font-mono">
                    v{fw.version}
                  </h3>
                  {i === 0 && (
                    <span className="rounded-full bg-green-100 dark:bg-green-900/30 px-2 py-0.5 text-[10px] font-medium text-green-700 dark:text-green-400">
                      atual
                    </span>
                  )}
                </div>
                <p className="mt-1 text-sm text-gray-500 dark:text-slate-400">
                  {formatSize(fw.size)} · {formatDate(fw.createdAt)}
                </p>
              </div>
              <button
                onClick={() => setDeleteTarget(fw)}
                className="ml-4 rounded px-3 py-1 text-sm text-red-600 dark:text-red-400 hover:bg-red-50 dark:hover:bg-red-900/30 transition-colors"
              >
                Remover
              </button>
            </div>
          ))}
        </div>
      )}

      <ConfirmModal
        open={!!deleteTarget}
        title="Remover firmware"
        message={`Remover firmware v${deleteTarget?.version}?`}
        confirmLabel="Remover"
        onConfirm={() => {
          if (deleteTarget) {
            deleteMutation.mutate(deleteTarget.id);
            setDeleteTarget(null);
          }
        }}
        onCancel={() => setDeleteTarget(null)}
      />
    </div>
  );
}
