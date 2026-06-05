import { Component, type ErrorInfo, type ReactNode } from "react";

interface Props {
  children: ReactNode;
}

interface State {
  hasError: boolean;
}

export default class ErrorBoundary extends Component<Props, State> {
  state: State = { hasError: false };

  static getDerivedStateFromError(): State {
    return { hasError: true };
  }

  componentDidCatch(error: Error, info: ErrorInfo) {
    console.error("[ErrorBoundary]", error, info.componentStack);
  }

  render() {
    if (!this.state.hasError) return this.props.children;

    return (
      <div className="flex min-h-screen items-center justify-center bg-[var(--color-surface)] px-4">
        <div className="max-w-md text-center">
          <div className="mb-4 text-6xl">:(</div>
          <h1 className="mb-2 text-xl font-semibold text-[var(--color-text-primary)]">
            Algo deu errado
          </h1>
          <p className="mb-6 text-[var(--color-text-secondary)]">
            Um erro inesperado aconteceu. Tente recarregar a página.
          </p>
          <button
            onClick={() => window.location.reload()}
            className="rounded-lg bg-[var(--color-accent)] px-6 py-2 font-medium text-white transition-colors hover:bg-[var(--color-accent-hover)]"
          >
            Recarregar
          </button>
        </div>
      </div>
    );
  }
}
