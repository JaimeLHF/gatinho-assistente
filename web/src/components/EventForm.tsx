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
      {error && (
        <p className="rounded-lg bg-red-50 dark:bg-red-900/30 p-3 text-sm text-red-600 dark:text-red-400">
          {error}
        </p>
      )}

      <div>
        <label
          htmlFor="title"
          className="block text-sm font-medium text-gray-700 dark:text-slate-300"
        >
          Titulo
        </label>
        <input
          id="title"
          required
          maxLength={200}
          value={title}
          onChange={(e) => setTitle(e.target.value)}
          className="mt-1 block w-full rounded-lg border border-gray-300 dark:border-slate-600 bg-white dark:bg-slate-800 px-3 py-2.5 text-sm text-gray-900 dark:text-white focus:border-indigo-500 dark:focus:border-indigo-400 focus:ring-2 focus:ring-indigo-500/20 focus:outline-none transition-colors"
        />
      </div>

      <div>
        <label
          htmlFor="description"
          className="block text-sm font-medium text-gray-700 dark:text-slate-300"
        >
          Descricao
        </label>
        <textarea
          id="description"
          rows={2}
          maxLength={2000}
          value={description}
          onChange={(e) => setDescription(e.target.value)}
          className="mt-1 block w-full rounded-lg border border-gray-300 dark:border-slate-600 bg-white dark:bg-slate-800 px-3 py-2.5 text-sm text-gray-900 dark:text-white focus:border-indigo-500 dark:focus:border-indigo-400 focus:ring-2 focus:ring-indigo-500/20 focus:outline-none transition-colors"
        />
      </div>

      <div>
        <label
          htmlFor="startsAt"
          className="block text-sm font-medium text-gray-700 dark:text-slate-300"
        >
          Data/Hora
        </label>
        <input
          id="startsAt"
          type="datetime-local"
          required
          value={startsAt}
          onChange={(e) => setStartsAt(e.target.value)}
          className="mt-1 block w-full rounded-lg border border-gray-300 dark:border-slate-600 bg-white dark:bg-slate-800 px-3 py-2.5 text-sm text-gray-900 dark:text-white focus:border-indigo-500 dark:focus:border-indigo-400 focus:ring-2 focus:ring-indigo-500/20 focus:outline-none transition-colors"
        />
      </div>

      <div className="grid grid-cols-2 gap-4">
        <div>
          <label
            htmlFor="durationMin"
            className="block text-sm font-medium text-gray-700 dark:text-slate-300"
          >
            Duracao (min)
          </label>
          <input
            id="durationMin"
            type="number"
            min="1"
            max="10080"
            value={durationMin}
            onChange={(e) => setDurationMin(e.target.value)}
            className="mt-1 block w-full rounded-lg border border-gray-300 dark:border-slate-600 bg-white dark:bg-slate-800 px-3 py-2.5 text-sm text-gray-900 dark:text-white focus:border-indigo-500 dark:focus:border-indigo-400 focus:ring-2 focus:ring-indigo-500/20 focus:outline-none transition-colors"
          />
        </div>
        <div>
          <label
            htmlFor="alertMinutesBefore"
            className="block text-sm font-medium text-gray-700 dark:text-slate-300"
          >
            Alerta antes (min)
          </label>
          <input
            id="alertMinutesBefore"
            type="number"
            min="0"
            max="1440"
            value={alertMinutesBefore}
            onChange={(e) => setAlertMinutesBefore(e.target.value)}
            className="mt-1 block w-full rounded-lg border border-gray-300 dark:border-slate-600 bg-white dark:bg-slate-800 px-3 py-2.5 text-sm text-gray-900 dark:text-white focus:border-indigo-500 dark:focus:border-indigo-400 focus:ring-2 focus:ring-indigo-500/20 focus:outline-none transition-colors"
          />
        </div>
      </div>

      <div className="flex justify-end gap-2 pt-2">
        <button
          type="button"
          onClick={onCancel}
          className="rounded-lg border border-gray-300 dark:border-slate-600 px-4 py-2 text-sm text-gray-700 dark:text-slate-300 hover:bg-gray-50 dark:hover:bg-slate-800 transition-colors"
        >
          Cancelar
        </button>
        <button
          type="submit"
          disabled={submitting}
          className="rounded-lg bg-indigo-600 dark:bg-indigo-500 px-4 py-2 text-sm font-semibold text-white hover:bg-indigo-700 dark:hover:bg-indigo-600 disabled:opacity-50 transition-colors"
        >
          {submitting ? "Salvando..." : submitLabel}
        </button>
      </div>
    </form>
  );
}
