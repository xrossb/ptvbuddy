import globals from "globals";
import js from "@eslint/js";
import json from "@eslint/json";
import { defineConfig } from "eslint/config";

export default defineConfig([
  {
    files: ["src/**/*.{js,mjs,cjs}"],
    plugins: { js },
    extends: ["js/recommended"],
  },
  {
    files: ["src/**/*.js"],
    languageOptions: {
      ecmaVersion: 6,
      sourceType: "commonjs",
      globals: {
        Pebble: "readonly",
        ...globals.browser,
      },
    },
  },
  {
    files: ["src/**/*.json"],
    plugins: { json },
    language: "json/json",
    extends: ["json/recommended"],
  },
]);
