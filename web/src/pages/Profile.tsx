import { useState, type FormEvent } from "react";
import { useAuth } from "../hooks/useAuth";
import * as authApi from "../api/auth";
import { AxiosError } from "axios";

export default function Profile() {
  const { user } = useAuth();

  const [name, setName] = useState(user?.name ?? "");
  const [email, setEmail] = useState(user?.email ?? "");
  const [profileMsg, setProfileMsg] = useState("");
  const [profileErr, setProfileErr] = useState("");
  const [saving, setSaving] = useState(false);

  const [currentPassword, setCurrentPassword] = useState("");
  const [newPassword, setNewPassword] = useState("");
  const [confirmPassword, setConfirmPassword] = useState("");
  const [pwdMsg, setPwdMsg] = useState("");
  const [pwdErr, setPwdErr] = useState("");
  const [changingPwd, setChangingPwd] = useState(false);

  async function handleProfile(e: FormEvent) {
    e.preventDefault();
    setProfileMsg("");
    setProfileErr("");
    setSaving(true);
    try {
      const updated = await authApi.updateProfile({ name, email });
      setName(updated.name);
      setEmail(updated.email);
      setProfileMsg("Perfil atualizado");
    } catch (err) {
      if (err instanceof AxiosError) {
        setProfileErr(err.response?.data?.error?.message ?? "Erro ao salvar");
      } else {
        setProfileErr("Erro inesperado");
      }
    } finally {
      setSaving(false);
    }
  }

  async function handlePassword(e: FormEvent) {
    e.preventDefault();
    setPwdMsg("");
    setPwdErr("");

    if (newPassword !== confirmPassword) {
      setPwdErr("As senhas nao coincidem");
      return;
    }

    setChangingPwd(true);
    try {
      await authApi.changePassword(currentPassword, newPassword);
      setPwdMsg("Senha alterada com sucesso");
      setCurrentPassword("");
      setNewPassword("");
      setConfirmPassword("");
    } catch (err) {
      if (err instanceof AxiosError) {
        setPwdErr(err.response?.data?.error?.message ?? "Erro ao alterar senha");
      } else {
        setPwdErr("Erro inesperado");
      }
    } finally {
      setChangingPwd(false);
    }
  }

  return (
    <div className="max-w-lg space-y-8">
      <div>
        <h1 className="text-2xl font-bold text-gray-900 dark:text-white">Perfil</h1>
        <p className="mt-1 text-sm text-gray-500 dark:text-slate-400">
          Gerencie suas informacoes pessoais.
        </p>
      </div>

      {/* Profile form */}
      <form
        onSubmit={handleProfile}
        className="rounded-2xl bg-white dark:bg-slate-900 p-6 shadow-sm dark:shadow-black/10 border border-gray-100 dark:border-slate-800 space-y-4"
      >
        <h2 className="text-lg font-semibold text-gray-900 dark:text-white">Dados pessoais</h2>

        {profileMsg && (
          <p className="rounded-lg bg-green-50 dark:bg-green-900/30 p-3 text-sm text-green-600 dark:text-green-400">
            {profileMsg}
          </p>
        )}
        {profileErr && (
          <p className="rounded-lg bg-red-50 dark:bg-red-900/30 p-3 text-sm text-red-600 dark:text-red-400">
            {profileErr}
          </p>
        )}

        <div>
          <label
            htmlFor="name"
            className="block text-sm font-medium text-gray-700 dark:text-slate-300"
          >
            Nome
          </label>
          <input
            id="name"
            required
            value={name}
            onChange={(e) => setName(e.target.value)}
            className="mt-1 block w-full rounded-lg border border-gray-300 dark:border-slate-600 bg-white dark:bg-slate-800 px-3 py-2 text-sm text-gray-900 dark:text-white focus:border-indigo-500 dark:focus:border-indigo-400 focus:ring-2 focus:ring-indigo-500/20 focus:outline-none transition-colors"
          />
        </div>

        <div>
          <label
            htmlFor="email"
            className="block text-sm font-medium text-gray-700 dark:text-slate-300"
          >
            Email
          </label>
          <input
            id="email"
            type="email"
            required
            value={email}
            onChange={(e) => setEmail(e.target.value)}
            className="mt-1 block w-full rounded-lg border border-gray-300 dark:border-slate-600 bg-white dark:bg-slate-800 px-3 py-2 text-sm text-gray-900 dark:text-white focus:border-indigo-500 dark:focus:border-indigo-400 focus:ring-2 focus:ring-indigo-500/20 focus:outline-none transition-colors"
          />
        </div>

        <button
          type="submit"
          disabled={saving}
          className="rounded-lg bg-indigo-600 dark:bg-indigo-500 px-4 py-2 text-sm font-semibold text-white hover:bg-indigo-700 dark:hover:bg-indigo-600 disabled:opacity-50 transition-colors"
        >
          {saving ? "Salvando..." : "Salvar"}
        </button>
      </form>

      {/* Password form */}
      <form
        onSubmit={handlePassword}
        className="rounded-2xl bg-white dark:bg-slate-900 p-6 shadow-sm dark:shadow-black/10 border border-gray-100 dark:border-slate-800 space-y-4"
      >
        <h2 className="text-lg font-semibold text-gray-900 dark:text-white">Alterar senha</h2>

        {pwdMsg && (
          <p className="rounded-lg bg-green-50 dark:bg-green-900/30 p-3 text-sm text-green-600 dark:text-green-400">
            {pwdMsg}
          </p>
        )}
        {pwdErr && (
          <p className="rounded-lg bg-red-50 dark:bg-red-900/30 p-3 text-sm text-red-600 dark:text-red-400">
            {pwdErr}
          </p>
        )}

        <div>
          <label
            htmlFor="currentPassword"
            className="block text-sm font-medium text-gray-700 dark:text-slate-300"
          >
            Senha atual
          </label>
          <input
            id="currentPassword"
            type="password"
            required
            value={currentPassword}
            onChange={(e) => setCurrentPassword(e.target.value)}
            className="mt-1 block w-full rounded-lg border border-gray-300 dark:border-slate-600 bg-white dark:bg-slate-800 px-3 py-2 text-sm text-gray-900 dark:text-white focus:border-indigo-500 dark:focus:border-indigo-400 focus:ring-2 focus:ring-indigo-500/20 focus:outline-none transition-colors"
          />
        </div>

        <div>
          <label
            htmlFor="newPassword"
            className="block text-sm font-medium text-gray-700 dark:text-slate-300"
          >
            Nova senha
          </label>
          <input
            id="newPassword"
            type="password"
            required
            minLength={8}
            value={newPassword}
            onChange={(e) => setNewPassword(e.target.value)}
            className="mt-1 block w-full rounded-lg border border-gray-300 dark:border-slate-600 bg-white dark:bg-slate-800 px-3 py-2 text-sm text-gray-900 dark:text-white focus:border-indigo-500 dark:focus:border-indigo-400 focus:ring-2 focus:ring-indigo-500/20 focus:outline-none transition-colors"
          />
        </div>

        <div>
          <label
            htmlFor="confirmPassword"
            className="block text-sm font-medium text-gray-700 dark:text-slate-300"
          >
            Confirmar nova senha
          </label>
          <input
            id="confirmPassword"
            type="password"
            required
            minLength={8}
            value={confirmPassword}
            onChange={(e) => setConfirmPassword(e.target.value)}
            className="mt-1 block w-full rounded-lg border border-gray-300 dark:border-slate-600 bg-white dark:bg-slate-800 px-3 py-2 text-sm text-gray-900 dark:text-white focus:border-indigo-500 dark:focus:border-indigo-400 focus:ring-2 focus:ring-indigo-500/20 focus:outline-none transition-colors"
          />
        </div>

        <button
          type="submit"
          disabled={changingPwd}
          className="rounded-lg bg-indigo-600 dark:bg-indigo-500 px-4 py-2 text-sm font-semibold text-white hover:bg-indigo-700 dark:hover:bg-indigo-600 disabled:opacity-50 transition-colors"
        >
          {changingPwd ? "Alterando..." : "Alterar senha"}
        </button>
      </form>
    </div>
  );
}
