import type { NextConfig } from "next";

const nextConfig: NextConfig = {
  output: "export",        // Static export para AWS S3
  trailingSlash: true,     // Necesario para S3 routing
  images: {
    unoptimized: true,     // S3 no tiene servidor de imágenes
  },
};

export default nextConfig;
