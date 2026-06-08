import { useState, useEffect, useRef, useCallback, useMemo } from "react";
import { useParams, useNavigate } from "react-router";
import { useQuery, useMutation, useQueryClient } from "@tanstack/react-query";
import * as devicesApi from "../api/devices";
import { useToast } from "../hooks/useToast";
import type { BgType } from "../types";
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

// Remap a pixel's HSL using ratio-based S/L (preserves shading) + additive H
interface RegionShift {
  dh: number;
  refS: number;
  refL: number;
  targetS: number;
  targetL: number;
}

function remapPixelHsl(
  ph: number,
  ps: number,
  pl: number,
  shift: RegionShift,
): [number, number, number] {
  const nh = (((ph + shift.dh) % 1) + 1) % 1;
  const ns = shift.refS > 0.01 ? Math.min(1, ps * (shift.targetS / shift.refS)) : shift.targetS;
  const nl =
    shift.refL > 0.01
      ? Math.min(1, pl * (shift.targetL / shift.refL))
      : Math.min(1, pl + shift.targetL);
  return [nh, ns, nl];
}

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

// --- Background presets ---

interface BgPreset {
  type: BgType;
  label: string;
  render: (ctx: CanvasRenderingContext2D, w: number, h: number, color: string) => void;
}

function drawSolidBg(ctx: CanvasRenderingContext2D, w: number, h: number, color: string) {
  ctx.fillStyle = color;
  ctx.fillRect(0, 0, w, h);
}

function drawStarsBg(ctx: CanvasRenderingContext2D, w: number, h: number) {
  ctx.fillStyle = "#0A0A2A";
  ctx.fillRect(0, 0, w, h);
  ctx.fillStyle = "#FFFFFF";
  // Deterministic stars using simple hash
  for (let i = 0; i < 40; i++) {
    const x = (i * 137 + 59) % w;
    const y = (i * 97 + 23) % h;
    const s = i % 3 === 0 ? 2 : 1;
    ctx.fillRect(x, y, s, s);
  }
}

function drawSkyBg(ctx: CanvasRenderingContext2D, w: number, h: number) {
  const grad = ctx.createLinearGradient(0, 0, 0, h);
  grad.addColorStop(0, "#5599DD");
  grad.addColorStop(0.7, "#AADDFF");
  grad.addColorStop(0.7, "#55AA44");
  grad.addColorStop(1, "#337722");
  ctx.fillStyle = grad;
  ctx.fillRect(0, 0, w, h);
}

function drawSunsetBg(ctx: CanvasRenderingContext2D, w: number, h: number) {
  const grad = ctx.createLinearGradient(0, 0, 0, h);
  grad.addColorStop(0, "#2A1B4E");
  grad.addColorStop(0.4, "#CC4455");
  grad.addColorStop(0.7, "#FF8844");
  grad.addColorStop(0.7, "#334433");
  grad.addColorStop(1, "#1A2A1A");
  ctx.fillStyle = grad;
  ctx.fillRect(0, 0, w, h);
}

function drawFieldBg(ctx: CanvasRenderingContext2D, w: number, h: number) {
  const grad = ctx.createLinearGradient(0, 0, 0, h);
  grad.addColorStop(0, "#88CC55");
  grad.addColorStop(0.65, "#66AA33");
  grad.addColorStop(0.65, "#775533");
  grad.addColorStop(1, "#554422");
  ctx.fillStyle = grad;
  ctx.fillRect(0, 0, w, h);
}

const BG_PRESETS: BgPreset[] = [
  { type: "solid", label: "Solido", render: drawSolidBg },
  { type: "stars", label: "Noite", render: (_ctx, w, h) => drawStarsBg(_ctx, w, h) },
  { type: "sky", label: "Ceu", render: (ctx, w, h) => drawSkyBg(ctx, w, h) },
  { type: "sunset", label: "Por do sol", render: (ctx, w, h) => drawSunsetBg(ctx, w, h) },
  { type: "field", label: "Campo", render: (ctx, w, h) => drawFieldBg(ctx, w, h) },
];

function renderBgThumb(preset: BgPreset, color: string): string {
  const tw = 64;
  const th = 34;
  const canvas = document.createElement("canvas");
  canvas.width = tw;
  canvas.height = th;
  const ctx = canvas.getContext("2d")!;
  preset.render(ctx, tw, th, color);
  return canvas.toDataURL();
}

// --- Cat model presets (based on reference pixel art sheet) ---

interface CatPreset {
  name: string;
  colors: ColorMap;
}

const PRESETS: CatPreset[] = [
  { name: "Original", colors: defaultColors() },
  {
    name: "Branco",
    colors: {
      outline: "#333333",
      stripes: "#CCCCCC",
      body: "#EEEEEE",
      belly: "#FFFFFF",
      eyes: "#55AA55",
      nose: "#FFAAAA",
    },
  },
  {
    name: "Cinza",
    colors: {
      outline: "#111122",
      stripes: "#555570",
      body: "#7777AA",
      belly: "#9999BB",
      eyes: "#55CC55",
      nose: "#887788",
    },
  },
  {
    name: "Prata",
    colors: {
      outline: "#111111",
      stripes: "#555566",
      body: "#AAAABB",
      belly: "#DDDDEE",
      eyes: "#55BB55",
      nose: "#BBAAAA",
    },
  },
  {
    name: "Preto",
    colors: {
      outline: "#000000",
      stripes: "#1A1A22",
      body: "#222233",
      belly: "#333344",
      eyes: "#DDBB22",
      nose: "#333333",
    },
  },
  {
    name: "Laranja",
    colors: {
      outline: "#1A1000",
      stripes: "#CC8822",
      body: "#FFAA33",
      belly: "#FFCC66",
      eyes: "#55AA55",
      nose: "#FFBBAA",
    },
  },
  {
    name: "Marrom",
    colors: {
      outline: "#111105",
      stripes: "#664422",
      body: "#996644",
      belly: "#BB8866",
      eyes: "#55AA55",
      nose: "#AA8866",
    },
  },
  {
    name: "Chocolate",
    colors: {
      outline: "#0A0A00",
      stripes: "#442211",
      body: "#664433",
      belly: "#886655",
      eyes: "#55AA55",
      nose: "#775544",
    },
  },
  {
    name: "Creme",
    colors: {
      outline: "#222211",
      stripes: "#CC9955",
      body: "#EEBB77",
      belly: "#FFDDAA",
      eyes: "#55AA55",
      nose: "#DDBB99",
    },
  },
  {
    name: "Cinza Tabby",
    colors: {
      outline: "#000000",
      stripes: "#333344",
      body: "#666677",
      belly: "#888899",
      eyes: "#CCAA22",
      nose: "#666666",
    },
  },
  {
    name: "Ruivo",
    colors: {
      outline: "#110500",
      stripes: "#DD6622",
      body: "#FF8833",
      belly: "#FFBB66",
      eyes: "#55AA55",
      nose: "#FFAA88",
    },
  },
  {
    name: "Caramelo",
    colors: {
      outline: "#111005",
      stripes: "#885522",
      body: "#AA7744",
      belly: "#CC9966",
      eyes: "#55AA55",
      nose: "#BB9977",
    },
  },
  {
    name: "Siames",
    colors: {
      outline: "#2A1A0A",
      stripes: "#6B4430",
      body: "#D4B896",
      belly: "#EAD8C0",
      eyes: "#4499DD",
      nose: "#C49A7A",
    },
  },
];

// Build shift table from color map
function buildShifts(colorMap: ColorMap): Record<number, RegionShift> {
  const shifts: Record<number, RegionShift> = {};
  for (const region of REGIONS) {
    const [refH, refS, refL] = hexToHsl(region.defaultColor);
    const [newH, newS, newL] = hexToHsl(colorMap[region.name as RegionName]);
    shifts[region.id] = { dh: newH - refH, refS, refL, targetS: newS, targetL: newL };
  }
  return shifts;
}

// Render the cat sprite into a small data URL with given colors
function renderPresetThumb(
  sprite: Uint8Array,
  regionMap: Uint8Array,
  presetColors: ColorMap,
): string {
  const canvas = document.createElement("canvas");
  canvas.width = CAT_W;
  canvas.height = CAT_H;
  const ctx = canvas.getContext("2d")!;
  const imageData = ctx.createImageData(CAT_W, CAT_H);
  const data = imageData.data;
  const shifts = buildShifts(presetColors);

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
    const [ph, ps, pl] = rgbToHsl(r, g, b);
    const [nh, ns, nl] = remapPixelHsl(ph, ps, pl, shift);
    const [nr, ng, nb] = hslToRgb(nh, ns, nl);
    data[si] = nr;
    data[si + 1] = ng;
    data[si + 2] = nb;
    data[si + 3] = 255;
  }

  ctx.putImageData(imageData, 0, 0);
  return canvas.toDataURL();
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
  const [bgOverride, setBgOverride] = useState<{ type: BgType; color: string } | null>(null);

  // Decode sprite data lazily (runs once)
  const spriteData = useMemo(() => {
    const sprite = decodeSpriteRGBA();
    const region = decodeRegionMap();
    return { sprite, region };
  }, []);

  // Load refs from decoded data
  useEffect(() => {
    spriteRef.current = spriteData.sprite;
    regionRef.current = spriteData.region;
  }, [spriteData]);

  // Generate preset thumbnails (runs once after sprite decode)
  const presetThumbs = useMemo(
    () => PRESETS.map((p) => renderPresetThumb(spriteData.sprite, spriteData.region, p.colors)),
    [spriteData],
  );

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
  const colors: ColorMap = useMemo(
    () =>
      overrides ??
      (existing
        ? {
            outline: existing.outline,
            stripes: existing.stripes,
            body: existing.body,
            belly: existing.belly,
            eyes: existing.eyes,
            nose: existing.nose,
          }
        : defaultColors()),
    [overrides, existing],
  );

  // Derive background: override > server > defaults
  const bgType: BgType = bgOverride?.type ?? (existing?.bgType as BgType | undefined) ?? "solid";
  const bgColor: string = bgOverride?.color ?? existing?.bgColor ?? "#000000";

  // Background thumbnail previews
  const bgThumbs = useMemo(() => BG_PRESETS.map((p) => renderBgThumb(p, bgColor)), [bgColor]);

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
        bgType,
        bgColor,
      }),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["customization", deviceId] });
      toast.success("Cores salvas! O gatinho sera atualizado em ate 60 segundos.");
    },
    onError: () => toast.error("Erro ao salvar cores"),
  });

  // Reset mutation
  const resetMutation = useMutation({
    mutationFn: () => devicesApi.deleteCustomization(deviceId!),
    onSuccess: () => {
      setOverrides(null);
      setBgOverride(null);
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

    // Draw background first
    const bgPreset = BG_PRESETS.find((p) => p.type === bgType);
    (bgPreset ?? BG_PRESETS[0]!).render(ctx, CANVAS_W, CANVAS_H, bgColor);

    const shifts = buildShifts(colors);

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

      const [ph, ps, pl] = rgbToHsl(r, g, b);
      const [nh, ns, nl] = remapPixelHsl(ph, ps, pl, shift);
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
  }, [colors, highlightRegion, bgType, bgColor]);

  useEffect(() => {
    renderCat();
  }, [renderCat]);

  function setRegionColor(name: RegionName, color: string) {
    setOverrides((prev) => ({ ...(prev ?? colors), [name]: color }));
  }

  function applyPreset(preset: CatPreset) {
    setOverrides({ ...preset.colors });
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
                  <label htmlFor={`color-${region.name}`} className="relative cursor-pointer">
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

          {/* Presets */}
          <div className="rounded-2xl bg-white dark:bg-slate-900 border border-gray-100 dark:border-slate-800 p-6 shadow-sm dark:shadow-black/10">
            <h2 className="text-lg font-semibold text-gray-900 dark:text-white mb-4">Modelos</h2>
            <div className="grid grid-cols-4 sm:grid-cols-5 lg:grid-cols-7 gap-3">
              {PRESETS.map((preset, i) => (
                <button
                  key={preset.name}
                  onClick={() => applyPreset(preset)}
                  className="group flex flex-col items-center gap-1 rounded-xl p-2 border border-gray-100 dark:border-slate-700 hover:bg-gray-50 dark:hover:bg-slate-800/50 hover:border-indigo-400 dark:hover:border-indigo-500 transition-all hover:scale-105"
                >
                  {presetThumbs[i] ? (
                    <img
                      src={presetThumbs[i]}
                      alt={preset.name}
                      className="w-12 h-14 object-contain"
                      style={{ imageRendering: "pixelated" }}
                    />
                  ) : (
                    <div className="w-12 h-14 bg-gray-200 dark:bg-slate-700 rounded animate-pulse" />
                  )}
                  <span className="text-[10px] leading-tight text-gray-500 dark:text-slate-400 group-hover:text-indigo-600 dark:group-hover:text-indigo-400 transition-colors">
                    {preset.name}
                  </span>
                </button>
              ))}
            </div>
          </div>

          {/* Background */}
          <div className="rounded-2xl bg-white dark:bg-slate-900 border border-gray-100 dark:border-slate-800 p-6 shadow-sm dark:shadow-black/10">
            <h2 className="text-lg font-semibold text-gray-900 dark:text-white mb-4">Fundo</h2>
            <div className="flex flex-wrap gap-3">
              {BG_PRESETS.map((preset, i) => (
                <button
                  key={preset.type}
                  onClick={() => setBgOverride({ type: preset.type, color: bgColor })}
                  className={`group flex flex-col items-center gap-1 rounded-xl p-2 border transition-all hover:scale-105 ${
                    bgType === preset.type
                      ? "border-indigo-500 dark:border-indigo-400 bg-indigo-50 dark:bg-indigo-900/30"
                      : "border-gray-100 dark:border-slate-700 hover:border-indigo-300 dark:hover:border-indigo-600"
                  }`}
                >
                  <img
                    src={bgThumbs[i]}
                    alt={preset.label}
                    className="w-16 h-9 rounded object-cover"
                    style={{ imageRendering: "pixelated" }}
                  />
                  <span
                    className={`text-[10px] leading-tight transition-colors ${
                      bgType === preset.type
                        ? "text-indigo-600 dark:text-indigo-400 font-medium"
                        : "text-gray-500 dark:text-slate-400"
                    }`}
                  >
                    {preset.label}
                  </span>
                </button>
              ))}
            </div>
            {bgType === "solid" && (
              <div className="flex items-center gap-3 mt-4">
                <label htmlFor="bg-color" className="relative cursor-pointer">
                  <input
                    id="bg-color"
                    type="color"
                    value={bgColor}
                    onChange={(e) => setBgOverride({ type: bgType, color: e.target.value })}
                    className="absolute inset-0 opacity-0 cursor-pointer w-10 h-10"
                  />
                  <div
                    className="w-10 h-10 rounded-lg border-2 border-gray-300 dark:border-slate-600 shadow-sm"
                    style={{ backgroundColor: bgColor }}
                  />
                </label>
                <div>
                  <span className="text-sm font-medium text-gray-900 dark:text-white">
                    Cor de fundo
                  </span>
                  <p className="text-xs text-gray-400 dark:text-slate-500 font-mono">
                    {bgColor.toUpperCase()}
                  </p>
                </div>
              </div>
            )}
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

          {/* Update notice */}
          <div className="flex items-start gap-2 rounded-lg border border-amber-200 dark:border-amber-800/50 bg-amber-50 dark:bg-amber-900/20 px-4 py-3">
            <span className="text-amber-500 mt-0.5 text-sm">&#9432;</span>
            <p className="text-xs text-amber-700 dark:text-amber-300">
              Apos salvar, as novas cores serao aplicadas automaticamente no dispositivo em ate 60
              segundos, no proximo ciclo de sincronizacao.
            </p>
          </div>
        </div>
      </div>
    </div>
  );
}
