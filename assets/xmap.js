const update_centered_equations = function () {
  // Read in the manifold specifications from the sliders
  const numObs = parseInt(document.getElementById("numObs").value);
  const E = parseInt(document.getElementById("E").value);
  const tau = parseInt(document.getElementById("tau").value);
  const p = parseInt(document.getElementById("p").value);
  const allowMissing = false;
  let library = parseInt(document.getElementById("library").value);

  // Construct the manifold and targets
  const M = manifold("u", numObs, E, tau, allowMissing, p).manifold;
  const targets = manifold("v", numObs, E, tau, allowMissing, p).targets;

  // Turn these into latex arrays
  const u_time_series = latexify_time_series("u", numObs);
  const v_time_series = latexify_time_series("v", numObs);

  const maniTex = latexify(M);

  // Update the library slider so it can't be larger than M_u
  const librarySlider = document.getElementById("library");

  if (library > M.length) {
    library = M.length;
    librarySlider.value = M.length;
    librarySlider.dispatchEvent(new Event("input"));
  }

  librarySlider.max = M.length;

  // Split the manifolds into library and prediction sets
  const libSet = M.slice(0, library);
  const libTargets = targets.slice(0, library);

  const predSet = M;
  const predTargets = targets;

  // Convert to latex to hand to mathjax
  const libSetTex = latexify(libSet);
  const predSetTex = latexify(predSet);

  const libFirstTex = latexify(libSet.slice(0, 1));
  const predFirstTex = latexify(predSet.slice(0, 1));
  const predFirstTargetTex = latexify(predTargets.slice(0, 1));

  const libTargetsTex = latexify(libTargets);
  const predTargetsTex = latexify(predTargets);
  const weightedSum =
    predSet.length > 0
      ? predSet[0].map((v, i) => `w_{1,${i}} \\times ${v}`).join(" + ")
      : "\\text{NA}";

  // Save the result to the page
  let eqnsToTypeset = [];

  const equations = document.querySelectorAll(".dynamic-equation");
  equations.forEach((eqn) => {
    const prevRenderedEquation = eqn.dataset.renderedEquation;
    const renderedEquation = eqn.dataset.equation
      .replace(/\${u_time_series}/, u_time_series)
      .replace(/\${v_time_series}/, v_time_series)
      .replace(/\${M_u}/, maniTex)
      .replace(/\${L}/, libSetTex)
      .replace(/\${P}/, predSetTex)
      .replace(/\${L_1}/, libFirstTex)
      .replace(/\${P_1}/, predFirstTex)
      .replace(/\${y_P_1}/, predFirstTargetTex)
      .replace(/\${y_L}/, libTargetsTex)
      .replace(/\${y_P}/, predTargetsTex)
      .replace(/\${yhat_P_1}/, weightedSum);

    if (prevRenderedEquation != renderedEquation) {
      eqnsToTypeset.push(eqn);
      MathJax.typesetClear([eqn]);
      eqn.innerHTML = renderedEquation;
      eqn.dataset.renderedEquation = renderedEquation;
    }
  });

  MathJax.typesetPromise(eqnsToTypeset);
};

const sliders = document.querySelectorAll(".slider-container input");

sliders.forEach((slider) =>
  slider.addEventListener("input", () => update_centered_equations())
);

update_centered_equations();
