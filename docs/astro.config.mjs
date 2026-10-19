import { satteri } from "@astrojs/markdown-satteri";
import starlight from "@astrojs/starlight";
import { defineConfig } from "astro/config";
import starlightLinksValidator from "starlight-links-validator";
import mdLinks from "./src/plugins/md-links.mjs";

const base = "/rpgmtranslate-qt";
const repo = "https://github.com/RPG-Maker-Translation-Tools/rpgmtranslate-qt";

export default defineConfig({
    site: "https://RPG-Maker-Translation-Tools.github.io",
    base,
    markdown: { processor: satteri({ mdastPlugins: [mdLinks(base)] }) },
    integrations: [
        starlight({
            title: "RPGMTranslate",
            description: "Documentation for RPGMTranslate",
            logo: { src: "./src/assets/rpgmtranslate-logo.png" },
            favicon: "/favicon.png",
            social: [{ icon: "github", label: "GitHub", href: repo }],
            editLink: { baseUrl: `${repo}/edit/main/docs/` },
            customCss: ["./src/styles/custom.css"],
            components: { Sidebar: "./src/components/Sidebar.astro" },
            plugins: [starlightLinksValidator()],
            defaultLocale: "root",
            locales: {
                root: { label: "English", lang: "en" },
                ru: { label: "Русский", lang: "ru" },
                zh: { label: "简体中文", lang: "zh-CN" },
            },
            sidebar: [
                { slug: "index", label: "Home", translations: { ru: "Главная", "zh-CN": "首页" } },
                { slug: "installation" },
                { slug: "text-editing" },
                { slug: "search" },
                { slug: "batch-processing" },
                { slug: "tasks" },
                { slug: "settings" },
                { slug: "glossary" },
                { slug: "rvpacker" },
                { slug: "git" },
                { slug: "asset-inspector" },
                { slug: "development" },
            ],
        }),
    ],
});
