import { Outlet, NavLink } from "react-router";

export default function Layout() {
  return (
    <div className="min-h-screen bg-gray-50">
      <header className="bg-white border-b border-gray-200">
        <div className="mx-auto max-w-5xl flex items-center justify-between px-4 h-14">
          <NavLink to="/" className="text-lg font-semibold text-gray-900">
            Gatinho Pet
          </NavLink>
          <nav className="flex items-center gap-4">
            <NavLink
              to="/"
              className={({ isActive }) =>
                `text-sm ${isActive ? "text-indigo-600 font-medium" : "text-gray-600 hover:text-gray-900"}`
              }
            >
              Dashboard
            </NavLink>
            <NavLink
              to="/events"
              className={({ isActive }) =>
                `text-sm ${isActive ? "text-indigo-600 font-medium" : "text-gray-600 hover:text-gray-900"}`
              }
            >
              Eventos
            </NavLink>
            <NavLink
              to="/devices"
              className={({ isActive }) =>
                `text-sm ${isActive ? "text-indigo-600 font-medium" : "text-gray-600 hover:text-gray-900"}`
              }
            >
              Dispositivos
            </NavLink>
          </nav>
        </div>
      </header>
      <main className="mx-auto max-w-5xl px-4 py-6">
        <Outlet />
      </main>
    </div>
  );
}
