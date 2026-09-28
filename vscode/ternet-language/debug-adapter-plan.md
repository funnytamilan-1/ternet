# Ternet Debugger Integration

The extension currently provides Run and Check commands.

A real debugger requires a Ternet Debug Adapter Protocol (DAP) backend in `tnc`, including breakpoints, stack frames, scopes, stepping, variable inspection and pause/resume. The extension does not advertise a fake debugger until those protocol operations are implemented.

Planned commands:
- `tnc debug <file.trn>`
- DAP initialize
- launch
- setBreakpoints
- continue
- next
- stepIn
- stepOut
- stackTrace
- scopes
- variables
- disconnect
