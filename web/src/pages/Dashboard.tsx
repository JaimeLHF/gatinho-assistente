import { useQuery } from "@tanstack/react-query";
import { Link } from "react-router";
import { listEvents } from "../api/events";
import { useAuth } from "../hooks/useAuth";

function formatDate(iso: string): string {
  return new Date(iso).toLocaleString("pt-BR", {
    day: "2-digit",
    month: "2-digit",
    hour: "2-digit",
    minute: "2-digit",
  });
}

export default function Dashboard() {
  const { user } = useAuth();

  const { data: events = [] } = useQuery({
    queryKey: ["events"],
    queryFn: () => listEvents(),
  });

  const pending = events.filter((e) => e.status === "PENDING");
  const upcoming = pending.slice(0, 5);

  return (
    <div>
      <h1 className="text-2xl font-bold text-gray-900 dark:text-white">Ola, {user?.name}</h1>

      <div className="mt-6 grid gap-4 sm:grid-cols-3">
        <div className="rounded-2xl bg-white dark:bg-slate-900 p-6 shadow-sm dark:shadow-black/10 border border-gray-100 dark:border-slate-800">
          <p className="text-sm text-gray-500 dark:text-slate-400">Total de eventos</p>
          <p className="mt-1 text-3xl font-bold text-gray-900 dark:text-white">{events.length}</p>
        </div>
        <div className="rounded-2xl bg-white dark:bg-slate-900 p-6 shadow-sm dark:shadow-black/10 border border-gray-100 dark:border-slate-800">
          <p className="text-sm text-gray-500 dark:text-slate-400">Pendentes</p>
          <p className="mt-1 text-3xl font-bold text-yellow-600">{pending.length}</p>
        </div>
        <div className="rounded-2xl bg-white dark:bg-slate-900 p-6 shadow-sm dark:shadow-black/10 border border-gray-100 dark:border-slate-800">
          <p className="text-sm text-gray-500 dark:text-slate-400">Concluidos</p>
          <p className="mt-1 text-3xl font-bold text-green-600">
            {events.filter((e) => e.status === "DONE").length}
          </p>
        </div>
      </div>

      <div className="mt-6">
        <div className="flex items-center justify-between">
          <h2 className="text-lg font-semibold text-gray-900 dark:text-white">Proximos eventos</h2>
          <Link
            to="/events"
            className="text-sm text-indigo-600 dark:text-indigo-400 hover:underline"
          >
            Ver todos
          </Link>
        </div>

        {upcoming.length === 0 ? (
          <p className="mt-3 text-sm text-gray-500 dark:text-slate-400">Nenhum evento pendente.</p>
        ) : (
          <ul className="mt-3 space-y-2">
            {upcoming.map((event) => (
              <li
                key={event.id}
                className="flex flex-col sm:flex-row sm:items-center justify-between gap-1 rounded-xl bg-white dark:bg-slate-900 p-4 shadow-sm dark:shadow-black/10 border border-gray-100 dark:border-slate-800"
              >
                <span className="font-medium text-gray-900 dark:text-white truncate">
                  {event.title}
                </span>
                <span className="text-sm text-gray-500 dark:text-slate-400 shrink-0">
                  {formatDate(event.startsAt)}
                </span>
              </li>
            ))}
          </ul>
        )}
      </div>
    </div>
  );
}
