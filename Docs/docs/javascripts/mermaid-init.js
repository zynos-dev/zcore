window.mermaidConfig = {
  startOnLoad: false,
  securityLevel: "loose",
  theme: "base",
  flowchart: {
    curve: "basis",
    nodeSpacing: 34,
    rankSpacing: 54,
    htmlLabels: true
  },
  themeVariables: {
    fontFamily: "Inter, Segoe UI, sans-serif",
    fontSize: "14px",
    background: "#030e1d",
    textColor: "#c8d3df",
    primaryColor: "#112c47",
    primaryTextColor: "#d9e8f5",
    primaryBorderColor: "#21cfff",
    secondaryColor: "#0d2238",
    secondaryTextColor: "#c8d3df",
    secondaryBorderColor: "#00a6ff",
    tertiaryColor: "#102941",
    tertiaryTextColor: "#c8d3df",
    tertiaryBorderColor: "#4de7ff",
    lineColor: "#4de7ff",
    edgeLabelBackground: "#0a1f33",
    clusterBkg: "#091a2c",
    clusterBorder: "#2a4a68",
    nodeBorder: "#21cfff",
    mainBkg: "#112c47"
  }
};

function renderMermaidDiagrams() {
  if (!window.mermaid) {
    return;
  }
  window.mermaid.initialize(window.mermaidConfig);
  window.mermaid.run({
    querySelector: ".mermaid"
  });
}

if (document.readyState === "loading") {
  document.addEventListener("DOMContentLoaded", renderMermaidDiagrams, { once: true });
} else {
  renderMermaidDiagrams();
}

document.addEventListener("md-content-updated", renderMermaidDiagrams);
