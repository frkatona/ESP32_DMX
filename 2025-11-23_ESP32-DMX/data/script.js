// DMX channels configuration
const channelDefs = [
    { ch: 1, label: "Yaw", type: "slider" },
    { ch: 2, label: "Tilt", type: "slider" },
    {
        ch: 3,
        label: "Color Wheel",
        type: "select",
        options: [
            { label: "Gobo / Default", value: 0 },
            { label: "Static Pair", value: 72 },
            { label: "Auto", value: 140 }
        ]
    },
    {
        ch: 4,
        label: "Gobo Wheel",
        type: "select",
        options: [
            { label: "Open White", value: 0 },
            { label: "Gobo 1", value: 8 },
            { label: "Gobo 2", value: 16 },
            { label: "Gobo 3", value: 24 },
            { label: "Gobo 4", value: 32 },
            { label: "Gobo 5", value: 40 },
            { label: "Gobo 6", value: 48 },
            { label: "Gobo 7", value: 56 },
            { label: "White Shake", value: 64 },
            { label: "Shake 1", value: 72 },
            { label: "Shake 2", value: 80 },
            { label: "Shake 3", value: 88 },
            { label: "Shake 4", value: 96 },
            { label: "Shake 5", value: 104 },
            { label: "Shake 6", value: 112 },
            { label: "Shake 7", value: 120 },
            { label: "Auto Gobo", value: 128 }
        ]
    },
    { ch: 5, label: "Strobe", type: "slider" },
    { ch: 6, label: "Master Dimmer", type: "slider" },
    { ch: 7, label: "Pan/Tilt Speed", type: "slider" },
    {
        ch: 8,
        label: "Auto Mode",
        type: "select",
        options: [
            { label: "Inactive", value: 0 },
            { label: "Auto", value: 135 },
            { label: "Sound Mode 3", value: 160 },
            { label: "Sound Mode 2", value: 185 },
            { label: "Sound Mode 1", value: 210 },
            { label: "Sound Mode 0", value: 235 }
        ]
    },
    {
        ch: 9,
        label: "Reset",
        type: "button",
        buttonLabel: "RESET DEVICE",
        value: 255
    },
];

const channelsContainer = document.getElementById("channels");
const statusEl = document.getElementById("status");
const allOffBtn = document.getElementById("allOffBtn");
const allFullBtn = document.getElementById("allFullBtn");

function setStatus(text, ok = true) {
    statusEl.textContent = text;
    statusEl.classList.toggle("ok", ok);
    statusEl.classList.toggle("err", !ok);
}

function sendDMXValue(ch, value) {
    // Basic GET /set?ch=<ch>&val=<value>
    fetch(`/set?ch=${encodeURIComponent(ch)}&val=${encodeURIComponent(value)}`)
        .then((res) => {
            if (!res.ok) throw new Error(res.statusText);
            setStatus(`Set ch ${ch} → ${value}`, true);
        })
        .catch((err) => {
            console.error(err);
            setStatus(`Error sending ch ${ch}`, false);
        });
}

function createChannelRow(def) {
    const row = document.createElement("div");
    row.className = "channel-row";

    const label = document.createElement("div");
    label.className = "ch-label";
    label.textContent = `Ch ${def.ch} — ${def.label}`;
    row.appendChild(label);

    let control;
    const valueBox = document.createElement("div");
    valueBox.className = "value-box";
    valueBox.textContent = "0";

    if (def.type === "select") {
        control = document.createElement("select");
        def.options.forEach(opt => {
            const option = document.createElement("option");
            option.value = opt.value;
            option.textContent = opt.label;
            control.appendChild(option);
        });

        control.addEventListener("change", () => {
            const v = Number(control.value);
            valueBox.textContent = v;
            sendDMXValue(def.ch, v);
        });

    } else if (def.type === "button") {
        control = document.createElement("button");
        control.className = "action-btn";
        control.textContent = def.buttonLabel || "Trigger";

        control.addEventListener("click", () => {
            const v = def.value || 255;
            valueBox.textContent = v;
            sendDMXValue(def.ch, v);
            // Optional: Reset visual feedback after a moment if it's a momentary action
            setTimeout(() => valueBox.textContent = "-", 1000);
        });
        valueBox.textContent = "-"; // Default state for button

    } else {
        // Default to slider
        control = document.createElement("input");
        control.type = "range";
        control.min = "0";
        control.max = "255";
        control.value = "0";
        control.step = "1";

        control.addEventListener("input", () => {
            valueBox.textContent = control.value;
        });

        control.addEventListener("change", () => {
            const v = Number(control.value) || 0;
            sendDMXValue(def.ch, v);
        });
    }

    row.appendChild(control);
    row.appendChild(valueBox);

    // Store references for bulk updates
    def.control = control;
    def.valueBox = valueBox;

    return row;
}

// Build UI
channelDefs.forEach((def) => {
    const row = createChannelRow(def);
    channelsContainer.appendChild(row);
});

// Bulk controls
allOffBtn.addEventListener("click", () => {
    channelDefs.forEach((def) => {
        if (def.type === "slider") {
            def.control.value = 0;
            def.valueBox.textContent = "0";
            sendDMXValue(def.ch, 0);
        } else if (def.type === "select") {
            // Try to select the first option (usually 0/Inactive)
            def.control.selectedIndex = 0;
            const v = Number(def.control.value);
            def.valueBox.textContent = v;
            sendDMXValue(def.ch, v);
        }
        // Skip buttons for bulk off
    });
});

allFullBtn.addEventListener("click", () => {
    channelDefs.forEach((def) => {
        if (def.type === "slider") {
            def.control.value = 255;
            def.valueBox.textContent = "255";
            sendDMXValue(def.ch, 255);
        }
        // Skip selects and buttons for bulk full (unsafe/unpredictable)
    });
});

// Optional: sync UI from /state endpoint
function fetchState() {
    fetch("/state")
        .then((res) => res.json())
        .then((json) => {
            channelDefs.forEach((def) => {
                const v = Number(json[def.ch]) || 0;
                if (def.type === "slider") {
                    def.control.value = v;
                } else if (def.type === "select") {
                    // Find closest option or exact match
                    // Simple exact match for now
                    def.control.value = v;
                }
                if (def.valueBox) def.valueBox.textContent = v;
            });
            setStatus("Synced from ESP32", true);
        })
        .catch(() => {
            // It's okay if /state isn't implemented
        });
}

setTimeout(fetchState, 1000);
