const fs = require("fs");
const path = require("path");

function createFileNode(basePath, itemPath, prefix) {
  const relativePath = prefix + path.relative(basePath, itemPath);
  const stats = fs.statSync(itemPath);
  const isDirectory = stats.isDirectory();

  const isAllowedFile = (filePath) => {
    const filesToIgnore = ["generateProjectManifest.js", "project-manifest.json", ".DS_Store"];
    if (filesToIgnore.includes(path.basename(filePath))) {
      return false;
    }
    const allowedExtensions = [".h", ".xml", ".c", ".ttf", ".bmp", ".jpg", ".jpeg", ".svg", ".png"];
    return allowedExtensions.some((ext) => filePath.endsWith(ext));
  };

  // Skip files that aren't in allowed extensions
  if (!isDirectory && !isAllowedFile(itemPath)) {
    return null;
  }

  const node = {
    name: path.basename(itemPath),
    path: relativePath,
    type: isDirectory ? "directory" : "file",
  };

  if (node.type === "file") {
    const content = fs.readFileSync(itemPath, "utf-8");
    if (content.trimStart().startsWith("<tests")) {
      node.isTest = true;
    }

    if (content.trimStart().startsWith("<component")) {
      node.isComponent = true;
    }

    if (content.trimStart().startsWith("<widget")) {
      node.isWidget = true;
    }
  }

  if (isDirectory) {
    const items = fs.readdirSync(itemPath);
    const children = items
      .map((item) => createFileNode(basePath, path.join(itemPath, item), prefix))
      .filter(Boolean) // Remove null entries
      .sort((a, b) => {
        if (a.type !== b.type) return a.type === "directory" ? -1 : 1;
        return a.name.localeCompare(b.name);
      });

    // Only include directory if it has matching files
    if (children.length > 0) {
      node.children = children;
      return node;
    }
    return null;
  }

  return node;
}

function generateManifest(directoryPath, prefix) {
  const manifest = createFileNode(directoryPath, directoryPath, prefix);
  fs.writeFileSync("project-manifest.json", JSON.stringify(manifest, null, 2));
}

// Get directory from command line args
const targetDir = process.argv[2];
if (!targetDir) {
  console.error("Please provide a directory path");
  console.error("Usage: node generateManifest.js <directory-path>");
  process.exit(1);
}

const prefix = process.argv[3] || "";

const fullPath = path.resolve(targetDir);
if (!fs.existsSync(fullPath)) {
  console.error(`Directory not found: ${fullPath}`);
  process.exit(1);
}

try {
  generateManifest(fullPath, prefix);
  console.log(`Manifest generated for: ${fullPath}`);
} catch (error) {
  console.error("Error generating manifest:", error);
  process.exit(1);
}
