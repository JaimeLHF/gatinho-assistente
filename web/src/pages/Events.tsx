import { useState } from "react";
import { useQuery, useMutation, useQueryClient } from "@tanstack/react-query";
import * as eventsApi from "../api/events";
import EventForm, { type EventFormData } from "../components/EventForm";
import ConfirmModal from "../components/ConfirmModal";
import { useToast } from "../hooks/useToast";
import type { Event } from "../types";

const STATUS_LABEL: Record<Event["status"], string> = {
  PENDING: "Pendente",
  DONE: "Concluido",
  DISMISSED: "Dispensado",
};

const STATUS_COLOR: Record<Event["status"], string> = {
  PENDING: "bg-yellow-100 text-yellow-800 dark:bg-yellow-900/30 dark:text-yellow-400",
  DONE: "bg-green-100 text-green-800 dark:bg-green-900/30 dark:text-green-400",
  DISMISSED: "bg-gray-100 text-gray-600 dark:bg-slate-700 dark:text-slate-400",
};

function formatDate(iso: string): string {
  return new Date(iso).toLocaleString("pt-BR", {
    day: "2-digit",
    month: "2-digit",
    year: "numeric",
    hour: "2-digit",
    minute: "2-digit",
  });
}

function buildPayload(data: EventFormData) {
  return {
    title: data.title,
    description: data.description || undefined,
    startsAt: new Date(data.startsAt).toISOString(),
    durationMin: data.durationMin ? Number(data.durationMin) : undefined,
    alertMinutesBefore: Number(data.alertMinutesBefore),
  };
}

export default function Events() {
  const queryClient = useQueryClient();
  const toast = useToast();
  const [showForm, setShowForm] = useState(false);
  const [editing, setEditing] = useState<Event | null>(null);
  const [deleteTarget, setDeleteTarget] = useState<Event | null>(null);

  // Filters
  const [filterFrom, setFilterFrom] = useState("");
  const [filterTo, setFilterTo] = useState("");

  const queryParams: Record<string, string> = {};
  if (filterFrom) queryParams.from = new Date(filterFrom).toISOString();
  if (filterTo) queryParams.to = new Date(filterTo).toISOString();

  const { data: events = [], isLoading } = useQuery({
    queryKey: ["events", queryParams],
    queryFn: () => eventsApi.listEvents(queryParams),
  });

  const createMutation = useMutation({
    mutationFn: (data: EventFormData) => eventsApi.createEvent(buildPayload(data)),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["events"] });
      setShowForm(false);
      toast.success("Evento criado");
    },
    onError: () => toast.error("Erro ao criar evento"),
  });

  const updateMutation = useMutation({
    mutationFn: ({ id, data }: { id: string; data: EventFormData }) =>
      eventsApi.updateEvent(id, buildPayload(data)),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["events"] });
      setEditing(null);
      toast.success("Evento atualizado");
    },
    onError: () => toast.error("Erro ao atualizar evento"),
  });

  const statusMutation = useMutation({
    mutationFn: ({ id, status }: { id: string; status: Event["status"] }) =>
      eventsApi.updateEvent(id, { status }),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["events"] });
      toast.success("Status atualizado");
    },
    onError: () => toast.error("Erro ao atualizar status"),
  });

  const deleteMutation = useMutation({
    mutationFn: (id: string) => eventsApi.deleteEvent(id),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["events"] });
      toast.success("Evento excluido");
    },
    onError: () => toast.error("Erro ao excluir evento"),
  });

  if (isLoading) {
    return <p className="text-gray-500 dark:text-slate-400">Carregando eventos...</p>;
  }

  if (showForm || editing) {
    return (
      <div>
        <h1 className="mb-4 text-2xl font-bold text-gray-900 dark:text-white">
          {editing ? "Editar evento" : "Novo evento"}
        </h1>
        <div className="max-w-lg rounded-2xl bg-white dark:bg-slate-900 p-6 shadow-xl dark:shadow-black/20 border border-gray-100 dark:border-slate-800">
          <EventForm
            initial={editing ?? undefined}
            submitLabel={editing ? "Salvar" : "Criar"}
            onCancel={() => {
              setShowForm(false);
              setEditing(null);
            }}
            onSubmit={async (data) => {
              if (editing) {
                await updateMutation.mutateAsync({ id: editing.id, data });
              } else {
                await createMutation.mutateAsync(data);
              }
            }}
          />
        </div>
      </div>
    );
  }

  return (
    <div>
      <div className="flex items-center justify-between">
        <h1 className="text-2xl font-bold text-gray-900 dark:text-white">Eventos</h1>
        <button
          onClick={() => setShowForm(true)}
          className="rounded-lg bg-indigo-600 dark:bg-indigo-500 px-4 py-2 text-sm font-semibold text-white hover:bg-indigo-700 dark:hover:bg-indigo-600 transition-colors"
        >
          Novo evento
        </button>
      </div>

      {/* Filters */}
      <div className="mt-4 flex flex-wrap items-end gap-3">
        <div>
          <label className="block text-xs font-medium text-gray-500 dark:text-slate-400">De</label>
          <input
            type="date"
            value={filterFrom}
            onChange={(e) => setFilterFrom(e.target.value)}
            className="mt-1 rounded-lg border border-gray-300 dark:border-slate-600 bg-white dark:bg-slate-800 px-2 py-1 text-sm text-gray-900 dark:text-white focus:border-indigo-500 dark:focus:border-indigo-400 focus:ring-1 focus:ring-indigo-500/20 focus:outline-none"
          />
        </div>
        <div>
          <label className="block text-xs font-medium text-gray-500 dark:text-slate-400">Ate</label>
          <input
            type="date"
            value={filterTo}
            onChange={(e) => setFilterTo(e.target.value)}
            className="mt-1 rounded-lg border border-gray-300 dark:border-slate-600 bg-white dark:bg-slate-800 px-2 py-1 text-sm text-gray-900 dark:text-white focus:border-indigo-500 dark:focus:border-indigo-400 focus:ring-1 focus:ring-indigo-500/20 focus:outline-none"
          />
        </div>
        {(filterFrom || filterTo) && (
          <button
            onClick={() => {
              setFilterFrom("");
              setFilterTo("");
            }}
            className="text-xs text-indigo-600 dark:text-indigo-400 hover:underline"
          >
            Limpar filtros
          </button>
        )}
      </div>

      {events.length === 0 ? (
        <p className="mt-6 text-center text-gray-500 dark:text-slate-400">
          Nenhum evento encontrado.
        </p>
      ) : (
        <div className="mt-4 space-y-3">
          {events.map((event) => (
            <div
              key={event.id}
              className="flex flex-col sm:flex-row sm:items-center justify-between gap-3 rounded-xl bg-white dark:bg-slate-900 p-4 shadow-sm dark:shadow-black/10 border border-gray-100 dark:border-slate-800"
            >
              <div className="min-w-0 flex-1">
                <div className="flex items-center gap-2">
                  <h3 className="truncate font-medium text-gray-900 dark:text-white">
                    {event.title}
                  </h3>
                  <span
                    className={`inline-block rounded-full px-2 py-0.5 text-xs font-medium ${STATUS_COLOR[event.status]}`}
                  >
                    {STATUS_LABEL[event.status]}
                  </span>
                </div>
                <p className="mt-1 text-sm text-gray-500 dark:text-slate-400">
                  {formatDate(event.startsAt)}
                  {event.durationMin != null && ` · ${event.durationMin} min`}
                  {` · alerta ${event.alertMinutesBefore} min antes`}
                </p>
                {event.description && (
                  <p className="mt-1 truncate text-sm text-gray-400 dark:text-slate-500">
                    {event.description}
                  </p>
                )}
              </div>

              <div className="flex shrink-0 items-center gap-1 sm:ml-4">
                {event.status === "PENDING" && (
                  <button
                    onClick={() => statusMutation.mutate({ id: event.id, status: "DONE" })}
                    className="rounded px-2 py-1 text-xs text-green-700 dark:text-green-400 hover:bg-green-50 dark:hover:bg-green-900/30"
                  >
                    Concluir
                  </button>
                )}
                <button
                  onClick={() => setEditing(event)}
                  className="rounded px-2 py-1 text-xs text-indigo-600 dark:text-indigo-400 hover:bg-indigo-50 dark:hover:bg-indigo-900/30"
                >
                  Editar
                </button>
                <button
                  onClick={() => setDeleteTarget(event)}
                  className="rounded px-2 py-1 text-xs text-red-600 dark:text-red-400 hover:bg-red-50 dark:hover:bg-red-900/30"
                >
                  Excluir
                </button>
              </div>
            </div>
          ))}
        </div>
      )}

      <ConfirmModal
        open={!!deleteTarget}
        title="Excluir evento"
        message={`Tem certeza que deseja excluir "${deleteTarget?.title}"?`}
        confirmLabel="Excluir"
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
