import { useState, useEffect, useRef, useCallback, useMemo } from "react";
import { useParams, useNavigate } from "react-router";
import { useQuery, useMutation, useQueryClient } from "@tanstack/react-query";
import * as devicesApi from "../api/devices";
import { useToast } from "../hooks/useToast";
import {
  CAT_W,
  CAT_H,
  REGIONS,
  REGION_TRANSPARENT,
  decodeSpriteRGBA,
  decodeRegionMap,
  type RegionId,
} from "../data/catSprite";

const SCALE = 5;
const CANVAS_W = CAT_W * SCALE;
const CANVAS_H = CAT_H * SCALE;

// --- Color helpers ---

function hexToHsl(hex: string): [number, number, number] {
  const r = parseInt(hex.slice(1, 3), 16) / 255;
  const g = parseInt(hex.slice(3, 5), 16) / 255;
  const b = parseInt(hex.slice(5, 7), 16) / 255;

  const max = Math.max(r, g, b);
  const min = Math.min(r, g, b);
  const l = (max + min) / 2;

  if (max === min) return [0, 0, l];

  const d = max - min;
  const s = l > 0.5 ? d / (2 - max - min) : d / (max + min);

  const h =
    max === r
      ? ((g - b) / d + (g < b ? 6 : 0)) / 6
      : max === g
        ? ((b - r) / d + 2) / 6
        : ((r - g) / d + 4) / 6;

  return [h, s, l];
}

function hslToRgb(h: number, s: number, l: number): [number, number, number] {
  if (s === 0) {
    const v = Math.round(l * 255);
    return [v, v, v];
  }

  const hue2rgb = (p: number, q: number, t: number) => {
    if (t < 0) t += 1;
    if (t > 1) t -= 1;
    if (t < 1 / 6) return p + (q - p) * 6 * t;
    if (t < 1 / 2) return q;
    if (t < 2 / 3) return p + (q - p) * (2 / 3 - t) * 6;
    return p;
  };

  const q = l < 0.5 ? l * (1 + s) : l + s - l * s;
  const p = 2 * l - q;
  return [
    Math.round(hue2rgb(p, q, h + 1 / 3) * 255),
    Math.round(hue2rgb(p, q, h) * 255),
    Math.round(hue2rgb(p, q, h - 1 / 3) * 255),
  ];
}

function rgbToHsl(r: number, g: number, b: number): [number, number, number] {
  return hexToHsl(
    `#${r.toString(16).padStart(2, "0")}${g.toString(16).padStart(2, "0")}${b.toString(16).padStart(2, "0")}`,
  );
}

type RegionName = "outline" | "stripes" | "body" | "belly" | "eyes" | "nose";
type ColorMap = Record<RegionName, string>;

function defaultColors(): ColorMap {
  return {
    outline: "#000000",
    stripes: "#5A3008",
    body: "#D55939",
    belly: "#F6AE62",
    eyes: "#FFFFFF",
    nose: "#EEDAA4",
  };
}

function getColor(map: ColorMap, name: string): string {
  return map[name as RegionName] ?? "#000000";
}

export default function Customize() {
  const { deviceId } = useParams<{ deviceId: string }>();
  const navigate = useNavigate();
  const queryClient = useQueryClient();
  const toast = useToast();
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const spriteRef = useRef<Uint8Array | null>(null);
  const regionRef = useRef<Uint8Array | null>(null);

  const [overrides, setOverrides] = useState<ColorMap | null>(null);
  const [highlightRegion, setHighlightRegion] = useState<RegionId | null>(null);

  // Load sprite data once
  useEffect(() => {
    spriteRef.current = decodeSpriteRGBA();
    regionRef.current = decodeRegionMap();
  }, []);

  // Fetch existing customization
  const { data: existing, isLoading } = useQuery({
    queryKey: ["customization", deviceId],
    queryFn: () => devicesApi.getCustomization(deviceId!),
    enabled: !!deviceId,
  });

  // Fetch device info for name display
  const { data: devices } = useQuery({
    queryKey: ["devices"],
    queryFn: devicesApi.listDevices,
  });

  const device = devices?.find((d) => d.id === deviceId);

  // Derive colors: user overrides > server data > defaults
  const colors: ColorMap = useMemo(() => overrides ?? (existing ? {
    outline: existing.outline,
    stripes: existing.stripes,
    body: existing.body,
    belly: existing.belly,
    eyes: existing.eyes,
    nose: existing.nose,
  } : defaultColors()), [overrides, existing]);

  // Save mutation
  const saveMutation = useMutation({
    mutationFn: () =>
      devicesApi.saveCustomization(deviceId!, {
        body: colors.body,
        stripes: colors.stripes,
        belly: colors.belly,
        outline: colors.outline,
        eyes: colors.eyes,
        nose: colors.nose,
      }),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["customization", deviceId] });
      toast.success("Cores salvas! O gatinho sera atualizado no proximo polling.");
    },
    onError: () => toast.error("Erro ao salvar cores"),
  });

  // Reset mutation
  const resetMutation = useMutation({
    mutationFn: () => devicesApi.deleteCustomization(deviceId!),
    onSuccess: () => {
      setOverrides(null);
      queryClient.invalidateQueries({ queryKey: ["customization", deviceId] });
      toast.success("Cores resetadas para o padrao");
    },
    onError: () => toast.error("Erro ao resetar cores"),
  });

  // Render the cat on canvas with color remapping
  const renderCat = useCallback(() => {
    const canvas = canvasRef.current;
    const sprite = spriteRef.current;
    const regionMap = regionRef.current;
    if (!canvas || !sprite || !regionMap) return;

    const ctx = canvas.getContext("2d")!;
    ctx.clearRect(0, 0, CANVAS_W, CANVAS_H);

    // Precompute HSL shifts per region
    const shifts: Record<
      number,
      { dh: number; ds: number; dl: number; refH: number; refS: number; refL: number }
    > = {};
    for (const region of REGIONS) {
      const [refH, refS, refL] = hexToHsl(region.defaultColor);
      const [newH, newS, newL] = hexToHsl(getColor(colors, region.name) ?? region.defaultColor);
      shifts[region.id] = {
        dh: newH - refH,
        ds: newS - refS,
        dl: newL - refL,
        refH,
        refS,
        refL,
      };
    }

    const imageData = ctx.createImageData(CAT_W, CAT_H);
    const data = imageData.data;

    for (let i = 0; i < CAT_W * CAT_H; i++) {
      const region = regionMap[i] as number;
      if (region === REGION_TRANSPARENT) continue;

      const si = i * 4;
      const r = sprite[si] as number;
      const g = sprite[si + 1] as number;
      const b = sprite[si + 2] as number;

      const shift = shifts[region];
      if (!shift) {
        data[si] = r;
        data[si + 1] = g;
        data[si + 2] = b;
        data[si + 3] = 255;
        continue;
      }

      // Get pixel HSL, apply shift
      const [ph, ps, pl] = rgbToHsl(r, g, b);
      const nh = (((ph + shift.dh) % 1) + 1) % 1;
      const ns = Math.max(0, Math.min(1, ps + shift.ds));
      const nl = Math.max(0, Math.min(1, pl + shift.dl));
      const [nr, ng, nb] = hslToRgb(nh, ns, nl);

      data[si] = nr;
      data[si + 1] = ng;
      data[si + 2] = nb;
      data[si + 3] = 255;
    }

    // Highlight selected region
    if (highlightRegion !== null) {
      for (let i = 0; i < CAT_W * CAT_H; i++) {
        const region = regionMap[i] as number;
        if (region === REGION_TRANSPARENT) continue;
        if (region !== highlightRegion) {
          const si = i * 4;
          data[si + 3] = 80; // dim non-highlighted pixels
        }
      }
    }

    // Draw scaled using temporary canvas for crisp pixels
    const tmpCanvas = document.createElement("canvas");
    tmpCanvas.width = CAT_W;
    tmpCanvas.height = CAT_H;
    tmpCanvas.getContext("2d")!.putImageData(imageData, 0, 0);

    ctx.imageSmoothingEnabled = false;
    ctx.drawImage(tmpCanvas, 0, 0, CANVAS_W, CANVAS_H);
  }, [colors, highlightRegion]);

  useEffect(() => {
    renderCat();
  }, [renderCat]);

  function setRegionColor(name: RegionName, color: string) {
    setOverrides((prev) => ({ ...(prev ?? colors), [name]: color }));
  }

  if (isLoading) {
    return <p className="text-gray-500 dark:text-slate-400">Carregando...</p>;
  }

  return (
    <div>
      <div className="flex items-center gap-3 mb-6">
        <button
          onClick={() => navigate("/devices")}
          className="rounded-lg border border-gray-300 dark:border-slate-600 px-3 py-1.5 text-sm text-gray-700 dark:text-slate-300 hover:bg-gray-50 dark:hover:bg-slate-800 transition-colors"
        >
          Voltar
        </button>
        <h1 className="text-2xl font-bold text-gray-900 dark:text-white">
          Personalizar {device?.name ?? "Gatinho"}
        </h1>
      </div>

      <div className="flex flex-col lg:flex-row gap-8">
        {/* Canvas preview */}
        <div className="flex flex-col items-center gap-4">
          <div className="rounded-2xl bg-gray-100 dark:bg-slate-800 p-6 border border-gray-200 dark:border-slate-700">
            <canvas
              ref={canvasRef}
              width={CANVAS_W}
              height={CANVAS_H}
              className="block"
              style={{ imageRendering: "pixelated" }}
            />
          </div>
          <p className="text-xs text-gray-400 dark:text-slate-500">
            Preview — frame idle ({CAT_W}x{CAT_H} @ {SCALE}x)
          </p>
        </div>

        {/* Color pickers */}
        <div className="flex-1 space-y-4">
          <div className="rounded-2xl bg-white dark:bg-slate-900 border border-gray-100 dark:border-slate-800 p-6 shadow-sm dark:shadow-black/10">
            <h2 className="text-lg font-semibold text-gray-900 dark:text-white mb-4">Cores</h2>
            <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
              {REGIONS.map((region) => (
                <div
                  key={region.id}
                  className="flex items-center gap-3 p-3 rounded-xl border border-gray-100 dark:border-slate-700 hover:bg-gray-50 dark:hover:bg-slate-800/50 transition-colors"
                  onMouseEnter={() => setHighlightRegion(region.id)}
                  onMouseLeave={() => setHighlightRegion(null)}
                >
                  <label
                    htmlFor={`color-${region.name}`}
                    className="relative cursor-pointer"
                  >
                    <input
                      id={`color-${region.name}`}
                      type="color"
                      value={getColor(colors, region.name)}
                      onChange={(e) => setRegionColor(region.name as RegionName, e.target.value)}
                      className="absolute inset-0 opacity-0 cursor-pointer w-10 h-10"
                    />
                    <div
                      className="w-10 h-10 rounded-lg border-2 border-gray-300 dark:border-slate-600 shadow-sm"
                      style={{ backgroundColor: getColor(colors, region.name) }}
                    />
                  </label>
                  <div className="flex-1 min-w-0">
                    <span className="text-sm font-medium text-gray-900 dark:text-white">
                      {region.label}
                    </span>
                    <p className="text-xs text-gray-400 dark:text-slate-500 font-mono">
                      {getColor(colors, region.name).toUpperCase()}
                    </p>
                  </div>
                  <button
                    onClick={() => setRegionColor(region.name as RegionName, region.defaultColor)}
                    className="text-xs text-gray-400 dark:text-slate-500 hover:text-gray-600 dark:hover:text-slate-300 transition-colors"
                    title="Restaurar cor padrao"
                  >
                    Reset
                  </button>
                </div>
              ))}
            </div>
          </div>

          {/* Actions */}
          <div className="flex gap-3">
            <button
              onClick={() => saveMutation.mutate()}
              disabled={saveMutation.isPending}
              className="rounded-lg bg-indigo-600 dark:bg-indigo-500 px-6 py-2.5 text-sm font-semibold text-white hover:bg-indigo-700 dark:hover:bg-indigo-600 disabled:opacity-50 transition-colors"
            >
              {saveMutation.isPending ? "Salvando..." : "Salvar cores"}
            </button>
            <button
              onClick={() => resetMutation.mutate()}
              disabled={resetMutation.isPending}
              className="rounded-lg border border-gray-300 dark:border-slate-600 px-6 py-2.5 text-sm font-semibold text-gray-700 dark:text-slate-300 hover:bg-gray-50 dark:hover:bg-slate-800 disabled:opacity-50 transition-colors"
            >
              Resetar padrao
            </button>
          </div>

          <p className="text-xs text-gray-400 dark:text-slate-500">
            As cores serao aplicadas no dispositivo no proximo ciclo de polling (~60s).
          </p>
        </div>
      </div>
    </div>
  );
}
