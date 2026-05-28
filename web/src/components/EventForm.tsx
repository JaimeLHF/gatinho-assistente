import { useState, type FormEvent } from "react";
import type { Event } from "../types";

interface EventFormProps {
  initial?: Event;
  onSubmit: (data: EventFormData) => Promise<void>;
  onCancel: () => void;
  submitLabel: string;
}

export interface EventFormData {
  title: string;
  description: string;
  startsAt: string;
  durationMin: string;
  alertMinutesBefore: string;
}

function toLocalDatetime(iso: string): string {
  const d = new Date(iso);
  const offset = d.getTimezoneOffset();
  const local = new Date(d.getTime() - offset * 60000);
  return local.toISOString().slice(0, 16);
}

export default function EventForm({ initial, onSubmit, onCancel, submitLabel }: EventFormProps) {
  const [title, setTitle] = useState(initial?.title ?? "");
  const [description, setDescription] = useState(initial?.description ?? "");
  const [startsAt, setStartsAt] = useState(initial ? toLocalDatetime(initial.startsAt) : "");
  const [durationMin, setDurationMin] = useState(initial?.durationMin?.toString() ?? "");
  const [alertMinutesBefore, setAlertMinutesBefore] = useState(
    initial?.alertMinutesBefore?.toString() ?? "5",
  );
  const [submitting, setSubmitting] = useState(false);
  const [error, setError] = useState("");

  async function handleSubmit(e: FormEvent) {
    e.preventDefault();
    setError("");
    setSubmitting(true);
    try {
      await onSubmit({ title, description, startsAt, durationMin, alertMinutesBefore });
    } catch {
      setError("Erro ao salvar evento");
    } finally {
      setSubmitting(false);
    }
  }

  return (
    <form onSubmit={handleSubmit} className="space-y-4">
      {error && <p className="rounded bg-red-50 p-2 text-sm text-red-600">{error}</p>}

      <div>
        <label htmlFor="title" className="block text-sm font-medium text-gray-700">
          Titulo
        </label>
        <input
          id="title"
          required
          maxLength={200}
          value={title}
          onChange={(e) => setTitle(e.target.value)}
          className="mt-1 block w-full rounded border border-gray-300 px-3 py-2 text-sm focus:border-indigo-500 focus:ring-1 focus:ring-indigo-500 focus:outline-none"
        />
      </div>

      <div>
        <label htmlFor="description" className="block text-sm font-medium text-gray-700">
          Descricao
        </label>
        <textarea
          id="description"
          rows={2}
          maxLength={2000}
          value={description}
          onChange={(e) => setDescription(e.target.value)}
          className="mt-1 block w-full rounded border border-gray-300 px-3 py-2 text-sm focus:border-indigo-500 focus:ring-1 focus:ring-indigo-500 focus:outline-none"
        />
      </div>

      <div>
        <label htmlFor="startsAt" className="block text-sm font-medium text-gray-700">
          Data/Hora
        </label>
        <input
          id="startsAt"
          type="datetime-local"
          required
          value={startsAt}
          onChange={(e) => setStartsAt(e.target.value)}
          className="mt-1 block w-full rounded border border-gray-300 px-3 py-2 text-sm focus:border-indigo-500 focus:ring-1 focus:ring-indigo-500 focus:outline-none"
        />
      </div>

      <div className="grid grid-cols-2 gap-4">
        <div>
          <label htmlFor="durationMin" className="block text-sm font-medium text-gray-700">
            Duracao (min)
          </label>
          <input
            id="durationMin"
            type="number"
            min="1"
            max="10080"
            value={durationMin}
            onChange={(e) => setDurationMin(e.target.value)}
            className="mt-1 block w-full rounded border border-gray-300 px-3 py-2 text-sm focus:border-indigo-500 focus:ring-1 focus:ring-indigo-500 focus:outline-none"
          />
        </div>
        <div>
          <label htmlFor="alertMinutesBefore" className="block text-sm font-medium text-gray-700">
            Alerta antes (min)
          </label>
          <input
            id="alertMinutesBefore"
            type="number"
            min="0"
            max="1440"
            value={alertMinutesBefore}
            onChange={(e) => setAlertMinutesBefore(e.target.value)}
            className="mt-1 block w-full rounded border border-gray-300 px-3 py-2 text-sm focus:border-indigo-500 focus:ring-1 focus:ring-indigo-500 focus:outline-none"
          />
        </div>
      </div>

      <div className="flex justify-end gap-2 pt-2">
        <button
          type="button"
          onClick={onCancel}
          className="rounded border border-gray-300 px-4 py-2 text-sm text-gray-700 hover:bg-gray-50"
        >
          Cancelar
        </button>
        <button
          type="submit"
          disabled={submitting}
          className="rounded bg-indigo-600 px-4 py-2 text-sm font-medium text-white hover:bg-indigo-700 disabled:opacity-50"
        >
          {submitting ? "Salvando..." : submitLabel}
        </button>
      </div>
    </form>
  );
}
