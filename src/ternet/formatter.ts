export class Formatter {
  format(source: string): string {
    const lines = source.split(/\r?\n/);
    const formattedLines: string[] = [];
    let indentLevel = 0;

    for (let rawLine of lines) {
      let line = rawLine.trim();

      if (!line) {
        formattedLines.push('');
        continue;
      }

      // Check for closing braces at start of line
      if (line.startsWith('}') || line.startsWith(']')) {
        indentLevel = Math.max(0, indentLevel - 1);
      }

      const indent = '  '.repeat(indentLevel);
      formattedLines.push(indent + line);

      // Check for opening braces that increase indent
      const openCount = (line.match(/[{[]/g) || []).length;
      const closeCount = (line.match(/[}\]]/g) || []).length;
      indentLevel += openCount - closeCount;
      if (indentLevel < 0) indentLevel = 0;
    }

    return formattedLines.join('\n').trim() + '\n';
  }
}
