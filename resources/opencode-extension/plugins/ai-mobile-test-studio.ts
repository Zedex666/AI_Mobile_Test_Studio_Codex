import { tool, type Plugin } from "@opencode-ai/plugin"
import net from "node:net"

const MAX_FRAME_BYTES = 1024 * 1024
const REQUEST_TIMEOUT_MS = 10_000
const OPERATION_TIMEOUT_MS = 35_000
const AUTOMATION_SYSTEM_INSTRUCTION = [
  "AI Mobile Test Studio exposes managed automation artifact directories through amts_automation_paths.",
  "When the user asks for an automation script, call amts_automation_paths with kind=script and deliver the final browser-openable HTML under its outputDirectory.",
  "When the user asks for a document or report, call amts_automation_paths with kind=report and deliver the final browser-openable HTML under its outputDirectory.",
  "The final entrypoint must be an .html or .htm file that opens directly without a build step; supporting CSS, JavaScript, and images may be placed under assets, while run logs and evidence belong under runs.",
  "Do not stop with only Markdown, a source snippet, or a build-only web project when one of these HTML deliverables was requested.",
].join(" ")

type JsonObject = Record<string, unknown>

type RpcResponse = {
  jsonrpc: "2.0"
  id: string
  result?: JsonObject
  error?: {
    code: number
    message: string
  }
}

function controlConfiguration() {
  const pipe = process.env.AI_MOBILE_TEST_STUDIO_CONTROL_PIPE
  const token = process.env.AI_MOBILE_TEST_STUDIO_CONTROL_TOKEN
  const protocol = process.env.AI_MOBILE_TEST_STUDIO_CONTROL_PROTOCOL
  if (!pipe || !token || protocol !== "1") {
    throw new Error(
      "AI Mobile Test Studio control API is unavailable. Start OpenCode from the Studio terminal.",
    )
  }
  const path = process.platform === "win32" && !pipe.startsWith("\\\\.\\pipe\\")
    ? `\\\\.\\pipe\\${pipe}`
    : pipe
  return { path, token }
}

function automationConfiguration(): JsonObject {
  const root = process.env.AI_MOBILE_TEST_STUDIO_AUTOMATION_ROOT
  const scripts = process.env.AI_MOBILE_TEST_STUDIO_AUTOMATION_SCRIPTS
  const reports = process.env.AI_MOBILE_TEST_STUDIO_AUTOMATION_REPORTS
  const assets = process.env.AI_MOBILE_TEST_STUDIO_AUTOMATION_ASSETS
  const runs = process.env.AI_MOBILE_TEST_STUDIO_AUTOMATION_RUNS
  if (!root || !scripts || !reports || !assets || !runs) {
    throw new Error(
      "Automation output directories are unavailable. Start OpenCode from the Studio terminal.",
    )
  }
  return {
    root,
    scripts,
    reports,
    assets,
    runs,
    htmlOutputContract: {
      scripts: "Write automation script frontends as .html files under scripts.",
      reports: "Write document and test reports as .html files under reports.",
      entrypoint: "The delivered HTML file must open directly in a default browser without a build step.",
    },
  }
}

function frame(message: JsonObject) {
  const payload = Buffer.from(JSON.stringify(message), "utf8")
  if (payload.length > MAX_FRAME_BYTES) {
    throw new Error("Control API request is too large.")
  }
  const output = Buffer.allocUnsafe(payload.length + 4)
  output.writeUInt32LE(payload.length, 0)
  payload.copy(output, 4)
  return output
}

async function rpc(method: string, parameters: JsonObject = {}, signal?: AbortSignal) {
  const { path, token } = controlConfiguration()
  const id = `${Date.now().toString(16)}-${Math.random().toString(16).slice(2)}`
  return await new Promise<JsonObject>((resolve, reject) => {
    const socket = net.createConnection(path)
    let buffer = Buffer.alloc(0)
    let settled = false
    const finish = (error?: Error, result?: JsonObject) => {
      if (settled) return
      settled = true
      clearTimeout(timeout)
      signal?.removeEventListener("abort", onAbort)
      socket.destroy()
      if (error) reject(error)
      else resolve(result ?? {})
    }
    const onAbort = () => finish(new Error("OpenCode canceled the Studio control request."))
    const timeout = setTimeout(
      () => finish(new Error(`Studio control request timed out: ${method}`)),
      REQUEST_TIMEOUT_MS,
    )
    signal?.addEventListener("abort", onAbort, { once: true })
    socket.once("connect", () => {
      socket.write(frame({
        jsonrpc: "2.0",
        id,
        method,
        params: { ...parameters, token },
      }))
    })
    socket.on("data", (data) => {
      buffer = Buffer.concat([buffer, data])
      if (buffer.length < 4) return
      const length = buffer.readUInt32LE(0)
      if (length === 0 || length > MAX_FRAME_BYTES) {
        finish(new Error("Studio returned an invalid response frame."))
        return
      }
      if (buffer.length < length + 4) return
      try {
        const response = JSON.parse(buffer.subarray(4, length + 4).toString("utf8")) as RpcResponse
        if (response.id !== id) {
          finish(new Error("Studio returned a response for a different request."))
        } else if (response.error) {
          finish(new Error(`${response.error.message} (${response.error.code})`))
        } else {
          finish(undefined, response.result ?? {})
        }
      } catch (error) {
        finish(error instanceof Error ? error : new Error(String(error)))
      }
    })
    socket.once("error", (error) => finish(error))
    socket.once("close", () => {
      if (!settled) finish(new Error("Studio closed the control connection without a response."))
    })
  })
}

async function waitForOperation(operation: JsonObject, signal?: AbortSignal) {
  const operationId = operation.operationId
  if (typeof operationId !== "string" || operation.status !== "running") return operation
  const deadline = Date.now() + OPERATION_TIMEOUT_MS
  while (Date.now() < deadline) {
    if (signal?.aborted) throw new Error("OpenCode canceled the Studio operation.")
    await new Promise((resolve) => setTimeout(resolve, 250))
    const current = await rpc("operation.get", { operationId }, signal)
    if (current.status !== "running") return current
  }
  throw new Error(`Studio operation timed out: ${operationId}`)
}

function output(title: string, value: JsonObject) {
  return {
    title,
    output: JSON.stringify(value, null, 2),
    metadata: value,
  }
}

export const AiMobileTestStudioPlugin: Plugin = async () => ({
  "experimental.chat.system.transform": async (_input, context) => {
    context.system.push(AUTOMATION_SYSTEM_INSTRUCTION)
  },
  tool: {
    amts_status: tool({
      description: "Read AI Mobile Test Studio status, active workspace, device state, and capabilities.",
      args: {},
      async execute(_args, context) {
        return output("AI Mobile Test Studio status", await rpc("studio.status", {}, context.abort))
      },
    }),
    amts_workspace_open: tool({
      description: "Open a stable AI Mobile Test Studio sidebar workspace in the desktop application.",
      args: {
        workspaceId: tool.schema.enum([
          "overview",
          "display",
          "mirroring",
          "terminal",
          "automation",
          "device-control",
          "packages",
          "apps",
          "files",
          "recovery",
          "performance",
          "layout",
          "logcat",
          "other",
          "process",
          "settings",
        ]),
      },
      async execute(args, context) {
        return output(
          "Studio workspace opened",
          await rpc("workspace.open", { workspaceId: args.workspaceId }, context.abort),
        )
      },
    }),
    amts_automation_paths: tool({
      description: "Return the Studio-managed directories for HTML automation scripts and reports. Use these paths when creating the final browser-openable HTML artifact.",
      args: {
        kind: tool.schema.enum(["script", "report"]).optional().default("script"),
      },
      async execute(args, _context) {
        const paths = automationConfiguration()
        return output(
          "Studio automation paths",
          {
            ...paths,
            outputDirectory: args.kind === "report" ? paths.reports : paths.scripts,
          },
        )
      },
    }),
    amts_device_refresh: tool({
      description: "Ask AI Mobile Test Studio to refresh its active Android device state.",
      args: {},
      async execute(_args, context) {
        return output("Device refresh requested", await rpc("device.refresh", {}, context.abort))
      },
    }),
    amts_device_read: tool({
      description: "Read a structured Android device snapshot or installed package list through Studio.",
      args: {
        view: tool.schema.enum(["snapshot", "apps"]),
        waitForCompletion: tool.schema.boolean().optional().default(true),
      },
      async execute(args, context) {
        const method = args.view === "apps" ? "device.apps.list" : "device.snapshot"
        const operation = await rpc(method, {}, context.abort)
        const result = args.waitForCompletion
          ? await waitForOperation(operation, context.abort)
          : operation
        return output("Studio device read", result)
      },
    }),
    amts_safe_action: tool({
      description: "Run a Studio-approved safe device action: keyEvent, launchApp, or stopApp.",
      args: {
        action: tool.schema.enum(["keyEvent", "launchApp", "stopApp"]),
        keyCode: tool.schema.string().optional(),
        packageName: tool.schema.string().optional(),
        waitForCompletion: tool.schema.boolean().optional().default(true),
      },
      async execute(args, context) {
        const operation = await rpc("device.action", {
          action: args.action,
          keyCode: args.keyCode,
          packageName: args.packageName,
        }, context.abort)
        const result = args.waitForCompletion
          ? await waitForOperation(operation, context.abort)
          : operation
        return output("Studio safe device action", result)
      },
    }),
    amts_operation_get: tool({
      description: "Read the latest state and result of an asynchronous Studio operation.",
      args: {
        operationId: tool.schema.string(),
      },
      async execute(args, context) {
        return output(
          "Studio operation",
          await rpc("operation.get", { operationId: args.operationId }, context.abort),
        )
      },
    }),
    amts_operation_cancel: tool({
      description: "Cancel a running asynchronous Studio operation.",
      args: {
        operationId: tool.schema.string(),
      },
      async execute(args, context) {
        return output(
          "Studio operation canceled",
          await rpc("operation.cancel", { operationId: args.operationId }, context.abort),
        )
      },
    }),
  },
})
