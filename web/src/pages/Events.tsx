import { useState } from "react";
import { useQuery, useMutation, useQueryClient } from "@tanstack/react-query";
import * as eventsApi from "../api/events";
import EventForm, { type EventFormData } from "../components/EventForm";
import type { Event } from "../types";

const STATUS_LABEL: Record<Event["status"], string> = {
  PENDING: "Pendente",
  DONE: "Concluido",
  DISMISSED: "Dispensado",
};

const STATUS_COLOR: Record<Event["status"], string> = {
  PENDING: "bg-yellow-100 text-yellow-800",
  DONE: "bg-green-100 text-green-800",
  DISMISSED: "bg-gray-100 text-gray-600",
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
  const [showForm, setShowForm] = useState(false);
  const [editing, setEditing] = useState<Event | null>(null);

  const { data: events = [], isLoading } = useQuery({
    queryKey: ["events"],
    queryFn: () => eventsApi.listEvents(),
  });

  const createMutation = useMutation({
    mutationFn: (data: EventFormData) => eventsApi.createEvent(buildPayload(data)),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["events"] });
      setShowForm(false);
    },
  });

  const updateMutation = useMutation({
    mutationFn: ({ id, data }: { id: string; data: EventFormData }) =>
      eventsApi.updateEvent(id, buildPayload(data)),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["events"] });
      setEditing(null);
    },
  });

  const statusMutation = useMutation({
    mutationFn: ({ id, status }: { id: string; status: Event["status"] }) =>
      eventsApi.updateEvent(id, { status }),
    onSuccess: () => queryClient.invalidateQueries({ queryKey: ["events"] }),
  });

  const deleteMutation = useMutation({
    mutationFn: (id: string) => eventsApi.deleteEvent(id),
    onSuccess: () => queryClient.invalidateQueries({ queryKey: ["events"] }),
  });

  if (isLoading) {
    return <p className="text-gray-500">Carregando eventos...</p>;
  }

  if (showForm || editing) {
    return (
      <div>
        <h1 className="mb-4 text-2xl font-bold text-gray-900">
          {editing ? "Editar evento" : "Novo evento"}
        </h1>
        <div className="max-w-lg rounded-lg bg-white p-6 shadow">
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
        <h1 className="text-2xl font-bold text-gray-900">Eventos</h1>
        <button
          onClick={() => setShowForm(true)}
          className="rounded bg-indigo-600 px-4 py-2 text-sm font-medium text-white hover:bg-indigo-700"
        >
          Novo evento
        </button>
      </div>

      {events.length === 0 ? (
        <p className="mt-6 text-center text-gray-500">Nenhum evento cadastrado.</p>
      ) : (
        <div className="mt-4 space-y-3">
          {events.map((event) => (
            <div
              key={event.id}
              className="flex items-center justify-between rounded-lg bg-white p-4 shadow-sm"
            >
              <div className="min-w-0 flex-1">
                <div className="flex items-center gap-2">
                  <h3 className="truncate font-medium text-gray-900">
                    {event.title}
                  </h3>
                  <span
                    className={`inline-block rounded-full px-2 py-0.5 text-xs font-medium ${STATUS_COLOR[event.status]}`}
                  >
                    {STATUS_LABEL[event.status]}
                  </span>
                </div>
                <p className="mt-1 text-sm text-gray-500">
                  {formatDate(event.startsAt)}
                  {event.durationMin != null && ` · ${event.durationMin} min`}
                  {` · alerta ${event.alertMinutesBefore} min antes`}
                </p>
                {event.description && (
                  <p className="mt-1 truncate text-sm text-gray-400">
                    {event.description}
                  </p>
                )}
              </div>

              <div className="ml-4 flex shrink-0 items-center gap-1">
                {event.status === "PENDING" && (
                  <button
                    onClick={() =>
                      statusMutation.mutate({ id: event.id, status: "DONE" })
                    }
                    className="rounded px-2 py-1 text-xs text-green-700 hover:bg-green-50"
                  >
                    Concluir
                  </button>
                )}
                <button
                  onClick={() => setEditing(event)}
                  className="rounded px-2 py-1 text-xs text-indigo-600 hover:bg-indigo-50"
                >
                  Editar
                </button>
                <button
                  onClick={() => {
                    if (confirm("Excluir este evento?")) {
                      deleteMutation.mutate(event.id);
                    }
                  }}
                  className="rounded px-2 py-1 text-xs text-red-600 hover:bg-red-50"
                >
                  Excluir
                </button>
              </div>
            </div>
          ))}
        </div>
      )}
    </div>
  );
}
