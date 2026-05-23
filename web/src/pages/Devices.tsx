import { useState, type FormEvent } from "react";
import { useQuery, useMutation, useQueryClient } from "@tanstack/react-query";
import * as devicesApi from "../api/devices";
import ConfirmModal from "../components/ConfirmModal";
import { useToast } from "../hooks/useToast";
import type { Device } from "../types";

function formatDate(iso: string): string {
  return new Date(iso).toLocaleString("pt-BR", {
    day: "2-digit",
    month: "2-digit",
    year: "numeric",
    hour: "2-digit",
    minute: "2-digit",
  });
}

function timeAgo(iso: string): string {
  const diff = Date.now() - new Date(iso).getTime();
  const minutes = Math.floor(diff / 60000);
  if (minutes < 1) return "agora";
  if (minutes < 60) return `${minutes} min atras`;
  const hours = Math.floor(minutes / 60);
  if (hours < 24) return `${hours}h atras`;
  const days = Math.floor(hours / 24);
  return `${days}d atras`;
}

export default function Devices() {
  const queryClient = useQueryClient();
  const toast = useToast();
  const [name, setName] = useState("");
  const [showForm, setShowForm] = useState(false);
  const [createdToken, setCreatedToken] = useState<string | null>(null);
  const [copied, setCopied] = useState(false);
  const [editing, setEditing] = useState<Device | null>(null);
  const [editName, setEditName] = useState("");
  const [revokeTarget, setRevokeTarget] = useState<Device | null>(null);

  const { data: devices = [], isLoading } = useQuery({
    queryKey: ["devices"],
    queryFn: devicesApi.listDevices,
  });

  const createMutation = useMutation({
    mutationFn: (deviceName: string) => devicesApi.createDevice(deviceName),
    onSuccess: (data) => {
      queryClient.invalidateQueries({ queryKey: ["devices"] });
      setCreatedToken(data.token);
      setName("");
      setShowForm(false);
      toast.success("Dispositivo criado");
    },
    onError: () => toast.error("Erro ao criar dispositivo"),
  });

  const updateMutation = useMutation({
    mutationFn: ({ id, newName }: { id: string; newName: string }) =>
      devicesApi.updateDevice(id, newName),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["devices"] });
      setEditing(null);
      toast.success("Dispositivo renomeado");
    },
    onError: () => toast.error("Erro ao renomear dispositivo"),
  });

  const deleteMutation = useMutation({
    mutationFn: (id: string) => devicesApi.deleteDevice(id),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["devices"] });
      toast.success("Dispositivo revogado");
    },
    onError: () => toast.error("Erro ao revogar dispositivo"),
  });

  function handleCreate(e: FormEvent) {
    e.preventDefault();
    if (name.trim()) {
      createMutation.mutate(name.trim());
    }
  }

  function handleEdit(e: FormEvent) {
    e.preventDefault();
    if (editing && editName.trim()) {
      updateMutation.mutate({ id: editing.id, newName: editName.trim() });
    }
  }

  function startEdit(device: Device) {
    setEditing(device);
    setEditName(device.name);
  }

  async function handleCopy() {
    if (createdToken) {
      await navigator.clipboard.writeText(createdToken);
      setCopied(true);
      setTimeout(() => setCopied(false), 2000);
    }
  }

  if (isLoading) {
    return <p className="text-gray-500">Carregando dispositivos...</p>;
  }

  return (
    <div>
      <div className="flex items-center justify-between">
        <h1 className="text-2xl font-bold text-gray-900">Dispositivos</h1>
        {!showForm && !createdToken && (
          <button
            onClick={() => setShowForm(true)}
            className="rounded bg-indigo-600 px-4 py-2 text-sm font-medium text-white hover:bg-indigo-700"
          >
            Novo dispositivo
          </button>
        )}
      </div>

      {createdToken && (
        <div className="mt-4 rounded-lg border border-amber-300 bg-amber-50 p-4">
          <p className="text-sm font-medium text-amber-800">
            Token criado com sucesso. Copie agora — ele nao sera exibido novamente.
          </p>
          <div className="mt-2 flex items-center gap-2">
            <code className="block flex-1 overflow-x-auto rounded bg-white px-3 py-2 font-mono text-xs text-gray-900 select-all">
              {createdToken}
            </code>
            <button
              onClick={handleCopy}
              className="shrink-0 rounded bg-amber-600 px-3 py-2 text-xs font-medium text-white hover:bg-amber-700"
            >
              {copied ? "Copiado!" : "Copiar"}
            </button>
          </div>
          <button
            onClick={() => setCreatedToken(null)}
            className="mt-3 text-sm text-amber-700 hover:underline"
          >
            Fechar
          </button>
        </div>
      )}

      {showForm && (
        <form onSubmit={handleCreate} className="mt-4 flex max-w-md items-end gap-2">
          <div className="flex-1">
            <label htmlFor="deviceName" className="block text-sm font-medium text-gray-700">
              Nome do dispositivo
            </label>
            <input
              id="deviceName"
              required
              value={name}
              onChange={(e) => setName(e.target.value)}
              placeholder="Ex: Sala de estar"
              className="mt-1 block w-full rounded border border-gray-300 px-3 py-2 text-sm focus:border-indigo-500 focus:ring-1 focus:ring-indigo-500 focus:outline-none"
            />
          </div>
          <button
            type="submit"
            disabled={createMutation.isPending}
            className="rounded bg-indigo-600 px-4 py-2 text-sm font-medium text-white hover:bg-indigo-700 disabled:opacity-50"
          >
            {createMutation.isPending ? "Criando..." : "Criar"}
          </button>
          <button
            type="button"
            onClick={() => setShowForm(false)}
            className="rounded border border-gray-300 px-4 py-2 text-sm text-gray-700 hover:bg-gray-50"
          >
            Cancelar
          </button>
        </form>
      )}

      {devices.length === 0 && !showForm ? (
        <p className="mt-6 text-center text-gray-500">Nenhum dispositivo cadastrado.</p>
      ) : (
        <div className="mt-4 space-y-3">
          {devices.map((device) => (
            <div
              key={device.id}
              className="flex items-center justify-between rounded-lg bg-white p-4 shadow-sm"
            >
              <div className="min-w-0 flex-1">
                {editing?.id === device.id ? (
                  <form onSubmit={handleEdit} className="flex items-center gap-2">
                    <input
                      autoFocus
                      required
                      value={editName}
                      onChange={(e) => setEditName(e.target.value)}
                      className="w-full rounded border border-gray-300 px-2 py-1 text-sm focus:border-indigo-500 focus:ring-1 focus:ring-indigo-500 focus:outline-none"
                    />
                    <button
                      type="submit"
                      disabled={updateMutation.isPending}
                      className="rounded bg-indigo-600 px-3 py-1 text-xs font-medium text-white hover:bg-indigo-700 disabled:opacity-50"
                    >
                      Salvar
                    </button>
                    <button
                      type="button"
                      onClick={() => setEditing(null)}
                      className="rounded border border-gray-300 px-3 py-1 text-xs text-gray-700 hover:bg-gray-50"
                    >
                      Cancelar
                    </button>
                  </form>
                ) : (
                  <>
                    <h3 className="font-medium text-gray-900">{device.name}</h3>
                    <p className="mt-1 text-sm text-gray-500">
                      Criado em {formatDate(device.createdAt)}
                      {device.lastSeenAt && (
                        <span className="ml-2 inline-flex items-center gap-1">
                          <span className="inline-block h-2 w-2 rounded-full bg-green-400" />
                          Visto {timeAgo(device.lastSeenAt)}
                        </span>
                      )}
                      {!device.lastSeenAt && (
                        <span className="ml-2 text-gray-400">· Nunca conectou</span>
                      )}
                    </p>
                  </>
                )}
              </div>

              {editing?.id !== device.id && (
                <div className="ml-4 flex shrink-0 items-center gap-1">
                  <button
                    onClick={() => startEdit(device)}
                    className="rounded px-3 py-1 text-sm text-indigo-600 hover:bg-indigo-50"
                  >
                    Editar
                  </button>
                  <button
                    onClick={() => setRevokeTarget(device)}
                    className="rounded px-3 py-1 text-sm text-red-600 hover:bg-red-50"
                  >
                    Revogar
                  </button>
                </div>
              )}
            </div>
          ))}
        </div>
      )}

      <ConfirmModal
        open={!!revokeTarget}
        title="Revogar dispositivo"
        message={`Tem certeza que deseja revogar "${revokeTarget?.name}"? O token deixara de funcionar.`}
        confirmLabel="Revogar"
        onConfirm={() => {
          if (revokeTarget) {
            deleteMutation.mutate(revokeTarget.id);
            setRevokeTarget(null);
          }
        }}
        onCancel={() => setRevokeTarget(null)}
      />
    </div>
  );
}
