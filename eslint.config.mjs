import globals from "globals";
import js from "@eslint/js";
import json from "@eslint/json";
import { defineConfig } from "eslint/config";
import tseslint from "typescript-eslint";

export default tseslint.config({
  files: ["src/pkjs/**/*.{js,mjs,cjs}"],
  extends: [js.configs.recommended, tseslint.configs.recommendedTypeChecked],
  languageOptions: {
    parserOptions: {
      projectService: true,
    },
  },
  rules: {
    "@typescript-eslint/no-require-imports": "off",
    "@typescript-eslint/no-unsafe-call": "off",
    "@typescript-eslint/no-unsafe-member-access": "off",
  },
});
