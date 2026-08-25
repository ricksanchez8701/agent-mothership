#!/usr/bin/env node
// Research loop orchestrator.
//
// Reads the GOAL every cycle, picks the next queued topic, runs the swarm
// (the agents in agents.json), synthesizes, and checkpoints every
// SAVE_INTERVAL minutes so work is never lost.
//
// Runners (the "brain" each agent calls):
//   --runner manual  (default)  write per-agent prompt files; the operator
//                               (me, Buffy, in-session) fills the outputs
//   --runner api     call an LLM API (set API_KEY + MODEL in env)
//   --runner cli     call an agent CLI (set AGENT_CLI in env)
//
// The loop is a harness. It does not think on its own between invocations;
// it prepares exactly what an agent must do and records what it did.

const fs = require('fs');
const path = require('path');

const ROOT = __dirname;
const CONFIG = {
  saveIntervalMs: 4 * 60 * 1000, // 4 minutes
  goalFile: path.join(ROOT, 'goal.md'),
  agentsFile: path.join(ROOT, 'agents.json'),
  queueFile: path.join(ROOT, 'queue.json'),
  forksFile: path.join(ROOT, 'forks.md'),
  dataDir: path.join(ROOT, 'data'),
  entriesDir: path.join(ROOT, 'entries'),
  findingsDir: path.join(ROOT, 'findings'),
  stateFile: path.join(ROOT, 'state.json'),
};

function readJSON(p) { return JSON.parse(fs.readFileSync(p, 'utf8')); }
function writeJSON(p, obj) { fs.writeFileSync(p, JSON.stringify(obj, null, 2) + '\n'); }
function read(p) { return fs.readFileSync(p, 'utf8'); }
function write(p, s) { fs.mkdirSync(path.dirname(p), { recursive: true }); fs.writeFileSync(p, s); }

function anchor() {
  const goal = read(CONFIG.goalFile);
  console.log('\n══════════════════════════════════════════════════');
  console.log(' GOAL (anchor — read every cycle)');
  console.log('══════════════════════════════════════════════════\n');
  console.log(goal);
  console.log('══════════════════════════════════════════════════\n');
  return goal;
}

function nextTopic(queue) {
  return (
    queue.topics.find((t) => t.status === 'queued') ||
    queue.topics.find((t) => t.status === 'in-progress')
  );
}

function agentPrompt(agent, topic, language) {
  return agent.prompt
    .replace(/\{language\}/g, language)
    .replace(/\{topic\}/g, topic);
}

function checkpoint(state, note) {
  state.lastSave = new Date().toISOString();
  state.notes = state.notes || [];
  state.notes.push({ t: state.lastSave, note });
  writeJSON(CONFIG.stateFile, state);
  console.log(`[checkpoint ${state.lastSave}] ${note}`);
}

function runAgentManual(agent, topic, language) {
  const promptPath = path.join(CONFIG.dataDir, topic.id, `${agent.id}.prompt.txt`);
  const out = path.join(CONFIG.dataDir, topic.id, `${agent.id}.md`);
  write(promptPath, agentPrompt(agent, topic.topic, language));
  console.log(`  [${agent.id}] prompt -> ${promptPath}`);
  console.log(`  [${agent.id}] expecting output at ${out}`);
  return out;
}

function runAgent(agent, topic, language, runner) {
  if (runner === 'manual') return runAgentManual(agent, topic, language);
  console.warn(`  [${agent.id}] runner '${runner}' not wired (api/cli). Using manual prompt files.`);
  return runAgentManual(agent, topic, language);
}

function synthesize(topic, runner) {
  const out = path.join(CONFIG.dataDir, topic.id, 'synthesis.md');
  if (runner === 'manual') {
    write(out, [
      `# Synthesis — ${topic.id} ${topic.topic}`,
      '',
      'Run the swarm synthesis (see swarm.md):',
      '1. Collect the 5 agent outputs in this folder.',
      '2. Find contradictions between L2/L3/L4.',
      '3. Extract the decisive statement ("if we change X, it works anywhere" or "locked in").',
      '4. Log any forks in forks.md.',
      '5. Write the entry to entries/ and update findings/.',
      '',
    ].join('\n'));
  }
  return out;
}

function main() {
  const i = process.argv.indexOf('--runner');
  const runner = i !== -1 ? process.argv[i + 1] : 'manual';

  anchor();
  const queue = readJSON(CONFIG.queueFile);
  const agents = readJSON(CONFIG.agentsFile).agents;
  const topic = nextTopic(queue);

  if (!topic) {
    console.log('No queued topics. Add one to queue.json.');
    return;
  }

  console.log(`CYCLE START — runner=${runner} — topic ${topic.id}: ${topic.topic}\n`);

  // Begin the 4-minute checkpoint timer.
  const state = { started: new Date().toISOString(), runner, topicId: topic.id, notes: [] };
  writeJSON(CONFIG.stateFile, state);
  const timer = setInterval(() => checkpoint(state, 'autosave (4 min)'), CONFIG.saveIntervalMs);

  // Run the swarm: every agent attacks the same topic.
  for (const agent of agents) {
    runAgent(agent, topic, queue.language, runner);
  }

  synthesize(topic, runner);
  checkpoint(state, `cycle done — ${topic.id} ${topic.topic}`);

  console.log('\nCycle complete. Outputs are under research/data/, entries under research/entries/.');
  console.log('Run: node research/loop.js --runner api|cli|manual\n');

  clearInterval(timer);
}

main();
